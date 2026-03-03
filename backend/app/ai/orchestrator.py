from __future__ import annotations

import logging
import re
import time
from collections import defaultdict, deque
from typing import TypedDict

from langchain_core.messages import AIMessage, HumanMessage, SystemMessage
from langchain_openai import ChatOpenAI
from langgraph.graph import END, START, StateGraph
from pydantic import BaseModel, SecretStr
from sqlalchemy import func, select
from sqlalchemy.ext.asyncio import AsyncSession

from ..core.config import settings
from ..models.ai_conversation import AIConversation
from ..models.device import Device
from ..models.room import Room
from ..models.server import Server
from ..models.switch_module import SwitchModule
from ..websocket.server import is_gateway_connected
from .memory import ConversationMemoryService
from .schemas import (
    ActionType,
    AIChatRequest,
    AIChatResponse,
    DeviceRef,
    ExecutionPlan,
    FinalResponse,
    Intent,
    MessageEntry,
    NLUHint,
    NLUResult,
    PersistedState,
    ResolvedEntities,
    RoomRef,
    SafetyDecision,
    ToolInput,
    ToolResult,
)
from .tools import (
    execute_device_action_tool,
    execute_status_query_tool,
    summarize_devices,
)

logger = logging.getLogger(__name__)


# ---------------------------------------------------------------------------
# Intent classification model (lightweight first LLM call)
# ---------------------------------------------------------------------------
class _IntentClassification(BaseModel):
    """Classify user message into one of three categories."""
    category: str  # "device_command", "status_query", or "conversation"
    reasoning: str = ""


_CLASSIFY_PROMPT = (
    "You are a smart-home intent classifier. "
    "Classify the user message into EXACTLY one category:\n"
    "  device_command  – user wants to turn on/off a device, set brightness, schedule something\n"
    "  status_query    – user asks about device status, counts, what's online/offline\n"
    "  conversation    – greeting, open-ended question, chit-chat, anything else\n"
    "Return the category and brief reasoning."
)


# ---------------------------------------------------------------------------
# System prompt for conversational replies
# ---------------------------------------------------------------------------
_SYSTEM_PROMPT = """You are a friendly and helpful smart-home assistant.

You can control devices (turn on/off, set brightness), check device status,
and answer general questions about the user's home.

Current home state:
{home_context}

Keep answers concise, warm, and helpful. When you perform device actions,
briefly confirm what you did. If you don't know something, say so honestly.
Do not invent device names or rooms that aren't in the home state above."""


class GraphState(TypedDict, total=False):
    session: AsyncSession
    request: AIChatRequest
    conversation: AIConversation
    persisted_state: PersistedState
    pending_plan: ExecutionPlan | None
    message_history: list[MessageEntry]
    intent: Intent
    hint: NLUHint
    entities: ResolvedEntities
    plan: ExecutionPlan
    decision: SafetyDecision
    tool_result: ToolResult
    final_response: FinalResponse


class AIOrchestrator:
    def __init__(self) -> None:
        self._memory = ConversationMemoryService()
        self._graph = self._build_graph()
        self._requests_by_conversation: dict[str, deque[float]] = defaultdict(deque)
        self._llm: ChatOpenAI | None = None
        llm_api_key = settings.llm_api_key
        if llm_api_key:
            llm_kwargs = {
                "api_key": SecretStr(llm_api_key),
                "model": settings.openai_model,
                "temperature": 0.7,
            }
            if settings.llm_api_base:
                llm_kwargs["base_url"] = settings.llm_api_base
            self._llm = ChatOpenAI(**llm_kwargs)

    def _build_graph(self):
        graph = StateGraph(GraphState)
        graph.add_node("load_context", self._load_context)
        graph.add_node("nlu", self._nlu)
        graph.add_node("resolver", self._resolver)
        graph.add_node("planner", self._planner)
        graph.add_node("safety", self._safety)
        graph.add_node("tool", self._tool)
        graph.add_node("result", self._result)
        graph.add_node("response", self._response)
        graph.add_node("persist", self._persist)

        graph.add_edge(START, "load_context")
        graph.add_edge("load_context", "nlu")
        graph.add_edge("nlu", "resolver")
        graph.add_edge("resolver", "planner")
        graph.add_edge("planner", "safety")
        graph.add_edge("safety", "tool")
        graph.add_edge("tool", "result")
        graph.add_edge("result", "response")
        graph.add_edge("response", "persist")
        graph.add_edge("persist", END)
        return graph.compile()

    async def run(self, session: AsyncSession, request: AIChatRequest) -> AIChatResponse:
        final_state = await self._graph.ainvoke({"session": session, "request": request})
        conversation = final_state["conversation"]
        return AIChatResponse(
            conversation_id=conversation.conversation_id,
            response=final_state["final_response"],
            expires_at=conversation.expires_at,
        )

    # ------------------------------------------------------------------
    # Pipeline nodes
    # ------------------------------------------------------------------

    async def _load_context(self, state: GraphState) -> GraphState:
        session = state.get("session")
        request = state.get("request")
        if session is None or request is None:
            raise ValueError("Missing required graph context for load_context")
        conversation = await self._memory.get_or_create(session, request.conversation_id)
        persisted_state = await self._memory.load_structured_state(conversation)
        pending_plan = await self._memory.get_pending_plan(conversation)
        message_history = await self._memory.load_message_history(conversation)
        return {
            "conversation": conversation,
            "persisted_state": persisted_state,
            "pending_plan": pending_plan,
            "message_history": message_history,
        }

    async def _nlu(self, state: GraphState) -> GraphState:
        request = state.get("request")
        if request is None:
            raise ValueError("Missing request in NLU state")
        pending_plan = state.get("pending_plan")

        # Handle confirmation flows first (no LLM needed)
        if request.confirm is False and pending_plan is not None:
            return {
                "intent": Intent(action=pending_plan.action, confidence=1.0),
                "hint": NLUHint(comp=pending_plan.comp, value=pending_plan.value),
            }

        if request.confirm is True and pending_plan is not None:
            return {
                "intent": Intent(action=pending_plan.action, confidence=1.0),
                "hint": NLUHint(comp=pending_plan.comp, value=pending_plan.value),
            }

        # Check for inventory queries via keyword matching
        inventory_query = self._is_inventory_query(request.message)

        if self._llm is None:
            # No LLM available — fall back to keyword heuristics
            if inventory_query:
                return {
                    "intent": Intent(action="status_check", confidence=1.0),
                    "hint": NLUHint(),
                }
            # Detect device commands via keywords
            cmd_result = self._detect_device_command(request.message)
            if cmd_result is not None:
                return cmd_result
            return {
                "intent": Intent(action="conversation", confidence=1.0),
                "hint": NLUHint(),
            }

        # --- Two-phase LLM approach ---
        # Phase 1: Classify intent
        try:
            classifier = self._llm.with_structured_output(_IntentClassification)
            classification = await classifier.ainvoke(
                [SystemMessage(content=_CLASSIFY_PROMPT), HumanMessage(content=request.message)]
            )
            if isinstance(classification, dict):
                category = str(classification.get("category", "conversation")).strip().lower()
            else:
                category = classification.category.strip().lower()
        except Exception:
            logger.warning("Intent classification failed, treating as conversation", exc_info=True)
            category = "conversation"

        if category == "conversation":
            return {
                "intent": Intent(action="conversation", confidence=1.0),
                "hint": NLUHint(),
            }

        if category == "status_query" or inventory_query:
            # Phase 2a: Extract structured slots for status queries
            try:
                extractor = self._llm.with_structured_output(NLUResult)
                result = await extractor.ainvoke(
                    [
                        SystemMessage(
                            content=(
                                "Extract structured smart-home query intent and slots. "
                                "Allowed actions: status_check. "
                                "Slots: room, device. "
                                "Return status_check with confidence 1.0."
                            )
                        ),
                        HumanMessage(content=request.message),
                    ]
                )
                if isinstance(result, dict):
                    result = NLUResult.model_validate(result)
                return {
                    "intent": Intent(action="status_check", confidence=1.0),
                    "hint": result.hint,
                }
            except Exception:
                return {
                    "intent": Intent(action="status_check", confidence=1.0),
                    "hint": NLUHint(),
                }

        # Phase 2b: Extract structured slots for device commands
        try:
            extractor = self._llm.with_structured_output(NLUResult)
            result = await extractor.ainvoke(
                [
                    SystemMessage(
                        content=(
                            "Extract structured smart-home command intent and slots. "
                            "Allowed actions: turn_on, turn_off, set_brightness, schedule_action, unknown. "
                            "Slots: room, device, comp, value, time. "
                            "Return unknown only if you truly cannot determine the action."
                        )
                    ),
                    HumanMessage(content=request.message),
                ]
            )
            if isinstance(result, dict):
                result = NLUResult.model_validate(result)
            # If extraction returns unknown/low-confidence, route to conversation
            if (
                result.intent.action == "unknown"
                or result.intent.confidence < settings.ai_confidence_threshold
            ):
                return {
                    "intent": Intent(action="conversation", confidence=1.0),
                    "hint": NLUHint(),
                }
            return {"intent": result.intent, "hint": result.hint}
        except Exception:
            logger.warning("NLU extraction failed, treating as conversation", exc_info=True)
            return {
                "intent": Intent(action="conversation", confidence=1.0),
                "hint": NLUHint(),
            }

    async def _resolver(self, state: GraphState) -> GraphState:
        session = state.get("session")
        intent = state.get("intent")
        hint = state.get("hint")
        persisted = state.get("persisted_state")
        if session is None or intent is None or hint is None or persisted is None:
            raise ValueError("Missing required state for resolver")

        # Conversation intents don't need entity resolution
        if intent.action == "conversation":
            return {
                "entities": ResolvedEntities(
                    room=None, devices=[], action="conversation"
                )
            }

        resolved_room = await self._resolve_room(
            session,
            room_hint=hint.room,
            fallback_room_id=persisted.entities.room.id if persisted.entities and persisted.entities.room else None,
        )

        resolved_devices = await self._resolve_devices(
            session,
            room=resolved_room,
            device_hint=hint.device,
            fallback_device_id=(
                persisted.entities.devices[0].id
                if persisted.entities and persisted.entities.devices
                else None
            ),
        )

        entities = ResolvedEntities(
            room=resolved_room,
            devices=resolved_devices,
            action=intent.action,
            value=hint.value,
            comp=hint.comp,
        )
        return {"entities": entities}

    async def _resolve_room(
        self,
        session: AsyncSession,
        room_hint: str | None,
        fallback_room_id: str | None,
    ) -> RoomRef | None:
        if room_hint:
            result = await session.execute(
                select(Room).where(
                    (Room.room_id == room_hint) | (func.lower(Room.name) == room_hint.lower())
                )
            )
            match = result.scalar_one_or_none()
            if match:
                return RoomRef(id=match.room_id, name=match.name)

        if fallback_room_id:
            match = await session.get(Room, fallback_room_id)
            if match:
                return RoomRef(id=match.room_id, name=match.name)
        return None

    async def _resolve_devices(
        self,
        session: AsyncSession,
        room: RoomRef | None,
        device_hint: str | None,
        fallback_device_id: str | None,
    ) -> list[DeviceRef]:
        if room is None:
            return []

        if device_hint:
            result = await session.execute(
                select(Device).where(
                    Device.room_id == room.id,
                    (Device.device_id == device_hint) | (func.lower(Device.name) == device_hint.lower()),
                )
            )
            rows = list(result.scalars().all())
            return [DeviceRef(id=row.device_id, name=row.name, type=row.device_type) for row in rows]

        # Requirement: apply to all devices in resolved room when no device is explicitly resolved.
        result = await session.execute(select(Device).where(Device.room_id == room.id))
        rows = list(result.scalars().all())
        if rows:
            return [DeviceRef(id=row.device_id, name=row.name, type=row.device_type) for row in rows]

        if fallback_device_id:
            fallback = await session.get(Device, fallback_device_id)
            if fallback and fallback.room_id == room.id:
                return [
                    DeviceRef(
                        id=fallback.device_id,
                        name=fallback.name,
                        type=fallback.device_type,
                    )
                ]
        return []

    async def _planner(self, state: GraphState) -> GraphState:
        request = state.get("request")
        entities = state.get("entities")
        if request is None or entities is None:
            raise ValueError("Missing required state for planner")
        pending_plan = state.get("pending_plan")

        if request.confirm is True and pending_plan is not None:
            return {"plan": pending_plan}

        plan = ExecutionPlan(
            action=entities.action,
            room=entities.room,
            devices=entities.devices,
            value=entities.value,
            comp=entities.comp or "Comp0",
            source="ai",
        )
        return {"plan": plan}

    async def _safety(self, state: GraphState) -> GraphState:
        request = state.get("request")
        intent = state.get("intent")
        plan = state.get("plan")
        if request is None or intent is None or plan is None:
            raise ValueError("Missing required state for safety")
        pending_plan = state.get("pending_plan")

        if self._is_rate_limited(request.conversation_id):
            return {"decision": SafetyDecision(decision="deny", reason_codes=["rate_limited"])}

        if request.confirm is False and pending_plan is not None:
            return {
                "decision": SafetyDecision(
                    decision="deny",
                    reason_codes=["confirmation_rejected"],
                )
            }

        if request.confirm is True and pending_plan is None:
            return {
                "decision": SafetyDecision(decision="clarify", reason_codes=["no_pending_confirmation"])
            }

        # Conversation intents always pass safety
        if intent.action == "conversation":
            return {"decision": SafetyDecision(decision="allow", reason_codes=[])}

        # Unknown / low-confidence → route to conversation instead of dead-end
        if intent.action == "unknown" or intent.confidence < settings.ai_confidence_threshold:
            return {"decision": SafetyDecision(decision="allow", reason_codes=["low_confidence_conversation"])}

        if plan.action in {"turn_on", "turn_off", "set_brightness"}:
            if plan.room is None:
                return {
                    "decision": SafetyDecision(decision="clarify", reason_codes=["room_required"])
                }
            if not plan.devices:
                return {
                    "decision": SafetyDecision(decision="clarify", reason_codes=["devices_required"])
                }

        if plan.action == "schedule_action" and settings.ai_high_risk_confirmation_required:
            if request.confirm is True and pending_plan is not None:
                return {"decision": SafetyDecision(decision="allow", reason_codes=[])}
            return {
                "decision": SafetyDecision(
                    decision="confirm",
                    reason_codes=["high_risk_schedule_requires_confirmation"],
                )
            }

        return {"decision": SafetyDecision(decision="allow", reason_codes=[])}

    async def _tool(self, state: GraphState) -> GraphState:
        session = state.get("session")
        decision = state.get("decision")
        plan = state.get("plan")
        request = state.get("request")
        if session is None or decision is None or plan is None or request is None:
            raise ValueError("Missing required state for tool")

        # Conversation intents skip tool execution
        if plan.action == "conversation":
            return {
                "tool_result": ToolResult(
                    status="no_action",
                    executed=False,
                    reason_codes=["conversation"],
                    items=[],
                )
            }

        if decision.decision != "allow":
            return {
                "tool_result": ToolResult(
                    status=(
                        "confirmation"
                        if decision.decision == "confirm"
                        else "clarification"
                        if decision.decision == "clarify"
                        else "denied"
                    ),
                    executed=False,
                    reason_codes=decision.reason_codes,
                    items=[],
                )
            }

        if plan.action == "status_check":
            tool_payload = ToolInput.model_validate({"plan": plan.model_dump()})
            result = await execute_status_query_tool(session, tool_payload, request.message)
            return {"tool_result": result}

        if plan.action == "schedule_action":
            return {
                "tool_result": ToolResult(
                    status="no_action",
                    executed=False,
                    reason_codes=["schedule_action_not_implemented"],
                    items=[],
                )
            }

        tool_payload = ToolInput.model_validate({"plan": plan.model_dump()})
        result = await execute_device_action_tool(session, tool_payload)
        return {"tool_result": result}

    async def _result(self, state: GraphState) -> GraphState:
        tool_result = state.get("tool_result")
        if tool_result is None:
            raise ValueError("Missing tool_result in result node")
        return {"tool_result": tool_result}

    async def _response(self, state: GraphState) -> GraphState:
        session = state.get("session")
        intent = state.get("intent")
        entities = state.get("entities")
        plan = state.get("plan")
        decision = state.get("decision")
        result = state.get("tool_result")
        request = state.get("request")
        if (
            session is None
            or intent is None
            or entities is None
            or plan is None
            or decision is None
            or result is None
            or request is None
        ):
            raise ValueError("Missing required state for response")
        message_history = state.get("message_history", [])

        # --- Denied ---
        if decision.decision == "deny":
            reply = "Action denied by safety policy."
            if "confirmation_rejected" in decision.reason_codes:
                reply = "Okay, I cancelled the pending action."
            reply = await self._generate_llm_reply(
                session, message_history, request.message,
                context_hint=reply,
            )
            return {
                "final_response": FinalResponse(
                    status="denied",
                    reply=reply,
                    requires_clarification=False,
                    requires_confirmation=False,
                    intent=intent,
                    entities=entities,
                    plan=plan,
                    result=result,
                )
            }

        # --- Clarification ---
        if decision.decision == "clarify":
            room_hint = entities.room.name if entities.room else None
            scope = f" in {room_hint}" if room_hint else ""
            hint_text = (
                f"The user needs clarification{scope}. "
                "Help them rephrase or ask what they'd like to do."
            )
            reply = await self._generate_llm_reply(
                session, message_history, request.message,
                context_hint=hint_text,
            )
            return {
                "final_response": FinalResponse(
                    status="clarification",
                    reply=reply,
                    requires_clarification=True,
                    requires_confirmation=False,
                    intent=intent,
                    entities=entities,
                    plan=plan,
                    result=result,
                )
            }

        # --- Confirmation ---
        if decision.decision == "confirm":
            reply = await self._generate_llm_reply(
                session, message_history, request.message,
                context_hint="Ask the user to confirm this action before proceeding.",
            )
            return {
                "final_response": FinalResponse(
                    status="confirmation",
                    reply=reply,
                    requires_clarification=False,
                    requires_confirmation=True,
                    intent=intent,
                    entities=entities,
                    plan=plan,
                    result=result,
                )
            }

        # --- Conversation (open-ended / chit-chat) ---
        if plan.action == "conversation" or "conversation" in result.reason_codes:
            reply = await self._generate_llm_reply(
                session, message_history, request.message,
            )
            return {
                "final_response": FinalResponse(
                    status="executed",
                    reply=reply,
                    requires_clarification=False,
                    requires_confirmation=False,
                    intent=intent,
                    entities=entities,
                    plan=plan,
                    result=result,
                )
            }

        # --- Executed device action ---
        if result.status == "executed":
            if plan.action == "status_check":
                context_hint = result.summary or "Here is the current system status."
                reply = await self._generate_llm_reply(
                    session, message_history, request.message,
                    context_hint=f"Summarise this status info naturally: {context_hint}",
                )
                return {
                    "final_response": FinalResponse(
                        status="executed",
                        reply=reply,
                        requires_clarification=False,
                        requires_confirmation=False,
                        intent=intent,
                        entities=entities,
                        plan=plan,
                        result=result,
                    )
                }

            room_name = plan.room.name if plan.room else "room"
            device_summary = summarize_devices(plan.devices)
            context_hint = f"Successfully executed {plan.action} in {room_name} for {device_summary}."
            reply = await self._generate_llm_reply(
                session, message_history, request.message,
                context_hint=context_hint,
            )
            return {
                "final_response": FinalResponse(
                    status="executed",
                    reply=reply,
                    requires_clarification=False,
                    requires_confirmation=False,
                    intent=intent,
                    entities=entities,
                    plan=plan,
                    result=result,
                )
            }

        # --- Fallback ---
        reply = await self._generate_llm_reply(
            session, message_history, request.message,
            context_hint="No specific action was performed. Let the user know.",
        )
        return {
            "final_response": FinalResponse(
                status=result.status,
                reply=reply,
                requires_clarification=False,
                requires_confirmation=False,
                intent=intent,
                entities=entities,
                plan=plan,
                result=result,
            )
        }

    async def _persist(self, state: GraphState) -> GraphState:
        session = state.get("session")
        request = state.get("request")
        conversation = state.get("conversation")
        intent = state.get("intent")
        entities = state.get("entities")
        plan = state.get("plan")
        decision = state.get("decision")
        result = state.get("tool_result")
        final_response = state.get("final_response")
        if (
            session is None
            or request is None
            or conversation is None
            or intent is None
            or entities is None
            or plan is None
            or decision is None
            or result is None
            or final_response is None
        ):
            raise ValueError("Missing required state for persist")
        message_history = state.get("message_history", [])

        persisted_state = PersistedState(
            intent=intent,
            entities=entities,
            plan=plan,
            decision=decision,
            result=result,
        )
        await self._memory.persist_structured_state(session, conversation, persisted_state)

        if decision.decision == "confirm":
            await self._memory.set_pending_plan(session, conversation, plan)
        else:
            await self._memory.set_pending_plan(session, conversation, None)

        await self._memory.update_context(
            session,
            conversation,
            room_id=entities.room.id if entities.room else None,
            device_id=entities.devices[0].id if entities.devices else None,
            intent=intent.action,
        )

        # Append user + assistant messages to history
        message_history.append(MessageEntry(role="user", content=request.message))
        message_history.append(MessageEntry(role="assistant", content=final_response.reply))
        await self._memory.save_message_history(session, conversation, message_history)

        await self._memory.append_audit_log(
            session,
            conversation_id=conversation.conversation_id,
            utterance=request.message,
            intent=intent.action,
            entities=entities.model_dump(mode="json"),
            confidence=intent.confidence,
            safety_decision=decision.decision,
            selected_action=plan.action,
            result=result.status,
            response_text=final_response.reply,
        )

        await session.refresh(conversation)
        return {"conversation": conversation}

    # ------------------------------------------------------------------
    # LLM reply generation
    # ------------------------------------------------------------------

    async def _generate_llm_reply(
        self,
        session: AsyncSession,
        history: list[MessageEntry],
        user_message: str,
        *,
        context_hint: str | None = None,
    ) -> str:
        """Build a conversational LLM reply using home context + history."""
        if self._llm is None:
            return self._fallback_reply(context_hint)

        try:
            home_context = await self._build_home_context(session)
            system_text = _SYSTEM_PROMPT.format(home_context=home_context)
            if context_hint:
                system_text += f"\n\nAdditional context: {context_hint}"

            messages: list = [SystemMessage(content=system_text)]

            # Add recent history (last 10 exchanges)
            for entry in history[-10:]:
                if entry.role == "user":
                    messages.append(HumanMessage(content=entry.content))
                elif entry.role == "assistant":
                    messages.append(AIMessage(content=entry.content))

            messages.append(HumanMessage(content=user_message))

            response = await self._llm.ainvoke(messages)
            text = response.content
            return text.strip() if isinstance(text, str) else str(text)
        except Exception:
            logger.warning("LLM reply generation failed", exc_info=True)
            return self._fallback_reply(context_hint)

    @staticmethod
    def _fallback_reply(context_hint: str | None = None) -> str:
        if context_hint:
            return context_hint
        return "I'm here to help with your smart home. What would you like to do?"

    async def _build_home_context(self, session: AsyncSession) -> str:
        """Query the DB to build a concise summary of the home for the system prompt."""
        lines: list[str] = []

        try:
            # Rooms
            rooms_result = await session.execute(select(Room))
            rooms = list(rooms_result.scalars().all())
            if rooms:
                lines.append(f"Rooms ({len(rooms)}): {', '.join(r.name for r in rooms)}")

            # Gateways / Servers
            servers_result = await session.execute(select(Server))
            servers = list(servers_result.scalars().all())
            for srv in servers:
                connected = is_gateway_connected(srv.server_id)
                status = "online" if connected else "offline"
                lines.append(f"Gateway '{srv.server_id}': {status}")

            # Devices per room
            for room in rooms:
                devices_result = await session.execute(
                    select(Device).where(Device.room_id == room.room_id)
                )
                devices = list(devices_result.scalars().all())
                if devices:
                    device_names = [f"{d.name} ({d.device_type})" for d in devices]
                    lines.append(f"  {room.name} devices: {', '.join(device_names)}")

                    # Switch states (through client_id)
                    for device in devices:
                        if not device.client_id:
                            continue
                        switches_result = await session.execute(
                            select(SwitchModule).where(
                                SwitchModule.client_id == device.client_id
                            )
                        )
                        switches = list(switches_result.scalars().all())
                        for sw in switches:
                            state_str = "ON" if sw.status else "OFF"
                            lines.append(
                                f"    {device.name}/{sw.comp_id}: {state_str} (brightness={sw.value})"
                            )
        except Exception:
            logger.warning("Failed to build home context", exc_info=True)
            lines.append("(home context unavailable)")

        return "\n".join(lines) if lines else "(no devices configured)"

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _is_rate_limited(self, conversation_id: str) -> bool:
        now = time.monotonic()
        bucket = self._requests_by_conversation[conversation_id]
        while bucket and now - bucket[0] > 60:
            bucket.popleft()
        if len(bucket) >= settings.ai_rate_limit_per_minute:
            return True
        bucket.append(now)
        return False

    # Regex patterns for keyword-based device-command detection (no-LLM path)
    _CMD_ON_RE = re.compile(
        r"\b(?:turn\s+on|switch\s+on|power\s+on|enable|activate)\b", re.IGNORECASE
    )
    _CMD_OFF_RE = re.compile(
        r"\b(?:turn\s+off|switch\s+off|power\s+off|disable|deactivate)\b", re.IGNORECASE
    )
    _CMD_BRIGHTNESS_RE = re.compile(
        r"\b(?:set\s+brightness|brightness\s+to|dim|dimmer|brighten)\b", re.IGNORECASE
    )
    _BRIGHTNESS_VALUE_RE = re.compile(r"\b(\d{1,4})\b")
    # Captures the room-like phrase after "the/in/in the" — greedy up to end or a stop word
    _ROOM_RE = re.compile(
        r"(?:the|in|in\s+the|of\s+the)\s+([a-z][a-z ]{1,30}?)(?:\s+lights?|\s+devices?|\s+switches?|\s+lamps?)?$",
        re.IGNORECASE,
    )
    # Fallback: after removing the command phrase, treat rest as room/device hint
    _ROOM_FALLBACK_RE = re.compile(
        r"(?:turn\s+on|turn\s+off|switch\s+on|switch\s+off|power\s+on|power\s+off|enable|disable|activate|deactivate|set\s+brightness\s+(?:to\s+)?\d*|dim|brighten)\s+(?:the\s+)?(.+)",
        re.IGNORECASE,
    )

    def _detect_device_command(self, message: str) -> dict | None:
        """Return NLU output dict if *message* matches a device-command keyword pattern."""
        text = message.strip()
        action: ActionType | None = None

        if self._CMD_OFF_RE.search(text):
            action = "turn_off"
        elif self._CMD_ON_RE.search(text):
            action = "turn_on"
        elif self._CMD_BRIGHTNESS_RE.search(text):
            action = "set_brightness"

        if action is None:
            return None

        # --- Extract room hint ---
        room_hint: str | None = None
        m = self._ROOM_RE.search(text)
        if m:
            room_hint = m.group(1).strip()
        else:
            m = self._ROOM_FALLBACK_RE.search(text)
            if m:
                room_hint = m.group(1).strip()
                # Remove trailing device-type words
                if room_hint:
                    room_hint = re.sub(
                        r"\s+(lights?|devices?|switches?|lamps?)$", "", room_hint, flags=re.IGNORECASE
                    ).strip() or None

        # --- Extract brightness value ---
        value: int | None = None
        if action == "set_brightness":
            vm = self._BRIGHTNESS_VALUE_RE.search(text)
            if vm:
                value = max(0, min(1000, int(vm.group(1))))

        return {
            "intent": Intent(action=action, confidence=1.0),
            "hint": NLUHint(room=room_hint, value=value),
        }

    def _is_inventory_query(self, message: str) -> bool:
        text = message.lower()
        has_inventory_target = any(
            token in text
            for token in (
                "device",
                "devices",
                "gateway",
                "gateways",
                "server",
                "servers",
                "switch",
                "switches",
                "module",
                "modules",
                "client",
                "clients",
            )
        )

        has_count_intent = (
            "how many" in text
            or "count" in text
            or "number of" in text
            or "total" in text
        )

        has_status_intent = any(
            phrase in text
            for phrase in (
                "what devices",
                "which devices",
                "what gateways",
                "which gateways",
                "what switches",
                "which switches",
                "connected",
                "online",
                "offline",
                "status",
                "available",
                "active",
            )
        )

        return has_inventory_target and (has_count_intent or has_status_intent)


orchestrator = AIOrchestrator()
