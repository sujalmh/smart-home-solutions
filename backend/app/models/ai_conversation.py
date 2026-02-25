from datetime import UTC, datetime

from sqlalchemy import JSON, Boolean, DateTime, String
from sqlalchemy.orm import Mapped, mapped_column

from ..db.base import Base


class AIConversation(Base):
    __tablename__ = "ai_conversations"

    conversation_id: Mapped[str] = mapped_column(String(64), primary_key=True)
    is_active: Mapped[bool] = mapped_column(Boolean, nullable=False, default=True, index=True)
    last_room_id: Mapped[str | None] = mapped_column(String(255), nullable=True)
    last_device_id: Mapped[str | None] = mapped_column(String(255), nullable=True)
    last_intent: Mapped[str | None] = mapped_column(String(64), nullable=True)
    pending_confirmation: Mapped[dict | None] = mapped_column(JSON, nullable=True)
    pending_plan: Mapped[dict | None] = mapped_column(JSON, nullable=True)
    structured_state: Mapped[dict | None] = mapped_column(JSON, nullable=True)
    expires_at: Mapped[datetime | None] = mapped_column(DateTime(timezone=True), nullable=True)
    created_at: Mapped[datetime] = mapped_column(
        DateTime(timezone=True), default=lambda: datetime.now(UTC), nullable=False
    )
    updated_at: Mapped[datetime] = mapped_column(
        DateTime(timezone=True),
        default=lambda: datetime.now(UTC),
        onupdate=lambda: datetime.now(UTC),
        nullable=False,
    )
