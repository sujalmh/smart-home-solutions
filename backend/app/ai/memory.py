from __future__ import annotations

from datetime import UTC, datetime, timedelta

from sqlalchemy.ext.asyncio import AsyncSession

from ..core.config import settings
from ..models.ai_audit_log import AIAuditLog
from ..models.ai_conversation import AIConversation
from .schemas import ExecutionPlan, PersistedState


def _now_utc() -> datetime:
    return datetime.now(UTC)


def _expires_at() -> datetime:
    return _now_utc() + timedelta(seconds=settings.ai_memory_ttl_seconds)


def _coerce_utc(value: datetime | None) -> datetime | None:
    if value is None:
        return None
    if value.tzinfo is None:
        return value.replace(tzinfo=UTC)
    return value.astimezone(UTC)


class ConversationMemoryService:
    async def get_or_create(self, session: AsyncSession, conversation_id: str) -> AIConversation:
        conversation = await session.get(AIConversation, conversation_id)
        if conversation is None:
            conversation = AIConversation(
                conversation_id=conversation_id,
                is_active=True,
                expires_at=_expires_at(),
                structured_state={},
            )
            session.add(conversation)
            await session.commit()
            await session.refresh(conversation)
            return conversation

        await self._apply_expiry(conversation)
        conversation.expires_at = _expires_at()
        await session.commit()
        await session.refresh(conversation)
        return conversation

    async def _apply_expiry(self, conversation: AIConversation) -> None:
        expires_at = _coerce_utc(conversation.expires_at)
        if expires_at and expires_at < _now_utc():
            conversation.last_room_id = None
            conversation.last_device_id = None
            conversation.last_intent = None
            conversation.pending_confirmation = None
            conversation.pending_plan = None
            conversation.structured_state = {}

    async def load_structured_state(self, conversation: AIConversation) -> PersistedState:
        raw = conversation.structured_state or {}
        return PersistedState.model_validate(raw)

    async def persist_structured_state(
        self,
        session: AsyncSession,
        conversation: AIConversation,
        state: PersistedState,
    ) -> AIConversation:
        conversation.structured_state = state.model_dump(mode="json")
        conversation.expires_at = _expires_at()
        await session.commit()
        await session.refresh(conversation)
        return conversation

    async def set_pending_plan(
        self,
        session: AsyncSession,
        conversation: AIConversation,
        plan: ExecutionPlan | None,
    ) -> AIConversation:
        conversation.pending_plan = None if plan is None else plan.model_dump(mode="json")
        conversation.expires_at = _expires_at()
        await session.commit()
        await session.refresh(conversation)
        return conversation

    async def get_pending_plan(self, conversation: AIConversation) -> ExecutionPlan | None:
        if not conversation.pending_plan:
            return None
        return ExecutionPlan.model_validate(conversation.pending_plan)

    async def update_context(
        self,
        session: AsyncSession,
        conversation: AIConversation,
        *,
        room_id: str | None,
        device_id: str | None,
        intent: str | None,
    ) -> AIConversation:
        conversation.last_room_id = room_id
        conversation.last_device_id = device_id
        conversation.last_intent = intent
        conversation.expires_at = _expires_at()
        await session.commit()
        await session.refresh(conversation)
        return conversation

    async def append_audit_log(
        self,
        session: AsyncSession,
        *,
        conversation_id: str,
        utterance: str,
        intent: str | None,
        entities: dict | None,
        confidence: float | None,
        safety_decision: str,
        selected_action: str | None,
        result: str | None,
        response_text: str | None,
    ) -> None:
        session.add(
            AIAuditLog(
                conversation_id=conversation_id,
                utterance=utterance,
                intent=intent,
                entities=entities,
                confidence=confidence,
                safety_decision=safety_decision,
                selected_action=selected_action,
                result=result,
                response_text=response_text,
            )
        )
        await session.commit()
