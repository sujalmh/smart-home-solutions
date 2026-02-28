from __future__ import annotations

import time
from collections import defaultdict, deque
from typing import TypedDict

from langchain_core.messages import HumanMessage, SystemMessage
from langchain_openai import ChatOpenAI
from langgraph.graph import END, START, StateGraph
from pydantic import SecretStr
from sqlalchemy import func, select
from sqlalchemy.ext.asyncio import AsyncSession

from ..core.config import settings
from ..models.ai_conversation import AIConversation
from ..models.device import Device
from ..models.room import Room
from .memory import ConversationMemoryService
from .schemas import (
    AIChatRequest,
    AIChatResponse,
    DeviceRef,
    ExecutionPlan,
    FinalResponse,
    Intent,
    NLUHint,
    NLUResult,
    PersistedState,
    ResolvedEntities,
    RoomRef,
    SafetyDecision,
    ToolInput,
    ToolResult,
)
from .tools import execute_device_action_tool, summarize_devices


class GraphState(TypedDict, total=False):
    session: AsyncSession
    request: AIChatRequest
    conversation: AIConversation
    persisted_state: PersistedState
    pending_plan: ExecutionPlan | None
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
                "temperature": 0,
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

    async def _load_context(self, state: GraphState) -> GraphState:
        session = state["session"]
        request = state["request"]
        conversation = await self._memory.get_or_create(session, request.conversation_id)
        persisted_state = await self._memory.load_structured_state(conversation)
        pending_plan = await self._memory.get_pending_plan(conversation)
        return {
            "conversation": conversation,
            "persisted_state": persisted_state,
            "pending_plan": pending_plan,
        }

    async def _nlu(self, state: GraphState) -> GraphState:
        request = state["request"]
        pending_plan = state.get("pending_plan")

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

        if self._llm is None:
            return {
                "intent": Intent(action="unknown", confidence=0.0),
                "hint": NLUHint(),
            }

        prompt = (
            "Extract only structured smart-home command intent and slots. "
            "Allowed actions: turn_on, turn_off, set_brightness, status_check, schedule_action, unknown. "
            "Slots: room, device, comp, value, time. "
            "Return unknown on uncertainty."
        )

        try:
            extractor = self._llm.with_structured_output(NLUResult)
            result = await extractor.ainvoke(
                [SystemMessage(content=prompt), HumanMessage(content=request.message)]
            )
            return {"intent": result.intent, "hint": result.hint}
        except Exception:
            return {
                "intent": Intent(action="unknown", confidence=0.0),
                "hint": NLUHint(),
            }

    async def _resolver(self, state: GraphState) -> GraphState:
        session = state["session"]
        intent = state["intent"]
        hint = state["hint"]
        persisted = state["persisted_state"]

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
        request = state["request"]
        pending_plan = state.get("pending_plan")
        entities = state["entities"]

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
        request = state["request"]
        intent = state["intent"]
        plan = state["plan"]
        pending_plan = state.get("pending_plan")

        if self._is_rate_limited(request.conversation_id):
            return {"decision": SafetyDecision(decision="deny", reason_codes=["rate_limited"]) }

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

        if intent.action == "unknown" or intent.confidence < settings.ai_confidence_threshold:
            return {
                "decision": SafetyDecision(decision="clarify", reason_codes=["low_confidence"])
            }

        if plan.action in {"turn_on", "turn_off", "set_brightness", "status_check"}:
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
        session = state["session"]
        decision = state["decision"]
        plan = state["plan"]

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
            return {
                "tool_result": ToolResult(
                    status="no_action",
                    executed=False,
                    reason_codes=["status_check_not_yet_connected_to_status_tool"],
                    items=[],
                )
            }

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
        # Explicit node preserved to keep structured stage boundary.
        return {"tool_result": state["tool_result"]}

    async def _response(self, state: GraphState) -> GraphState:
        intent = state["intent"]
        entities = state["entities"]
        plan = state["plan"]
        decision = state["decision"]
        result = state["tool_result"]

        if decision.decision == "deny":
            reply = "Action denied by safety policy."
            if "confirmation_rejected" in decision.reason_codes:
                reply = "Okay, I cancelled the pending action."
            final = FinalResponse(
                status="denied",
                reply=reply,
                requires_clarification=False,
                requires_confirmation=False,
                intent=intent,
                entities=entities,
                plan=plan,
                result=result,
            )
            return {"final_response": final}

        if decision.decision == "clarify":
            final = FinalResponse(
                status="clarification",
                reply="I need more details to continue. Please specify room and/or device.",
                requires_clarification=True,
                requires_confirmation=False,
                intent=intent,
                entities=entities,
                plan=plan,
                result=result,
            )
            return {"final_response": final}

        if decision.decision == "confirm":
            final = FinalResponse(
                status="confirmation",
                reply="Please confirm this action.",
                requires_clarification=False,
                requires_confirmation=True,
                intent=intent,
                entities=entities,
                plan=plan,
                result=result,
            )
            return {"final_response": final}

        if result.status == "executed":
            room_name = plan.room.name if plan.room else "room"
            device_summary = summarize_devices(plan.devices)
            final = FinalResponse(
                status="executed",
                reply=f"Done. Executed {plan.action} in {room_name} for {device_summary}.",
                requires_clarification=False,
                requires_confirmation=False,
                intent=intent,
                entities=entities,
                plan=plan,
                result=result,
            )
            return {"final_response": final}

        final = FinalResponse(
            status=result.status,
            reply="No executable action was performed.",
            requires_clarification=False,
            requires_confirmation=False,
            intent=intent,
            entities=entities,
            plan=plan,
            result=result,
        )
        return {"final_response": final}

    async def _persist(self, state: GraphState) -> GraphState:
        session = state["session"]
        request = state["request"]
        conversation = state["conversation"]
        intent = state["intent"]
        entities = state["entities"]
        plan = state["plan"]
        decision = state["decision"]
        result = state["tool_result"]
        final_response = state["final_response"]

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

    def _is_rate_limited(self, conversation_id: str) -> bool:
        now = time.monotonic()
        bucket = self._requests_by_conversation[conversation_id]
        while bucket and now - bucket[0] > 60:
            bucket.popleft()
        if len(bucket) >= settings.ai_rate_limit_per_minute:
            return True
        bucket.append(now)
        return False


orchestrator = AIOrchestrator()
