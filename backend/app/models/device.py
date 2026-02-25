from datetime import UTC, datetime

from sqlalchemy import Boolean, DateTime, ForeignKey, String
from sqlalchemy.orm import Mapped, mapped_column

from ..db.base import Base


class Device(Base):
    __tablename__ = "devices"

    device_id: Mapped[str] = mapped_column(String(255), primary_key=True)
    room_id: Mapped[str] = mapped_column(
        String(255), ForeignKey("rooms.room_id"), nullable=False, index=True
    )
    server_id: Mapped[str | None] = mapped_column(
        String(255), ForeignKey("servers.server_id"), nullable=True, index=True
    )
    client_id: Mapped[str | None] = mapped_column(
        String(255), ForeignKey("clients.client_id"), nullable=True, unique=True
    )
    name: Mapped[str] = mapped_column(String(255), nullable=False)
    device_type: Mapped[str] = mapped_column(
        String(64), nullable=False, default="switch"
    )
    is_active: Mapped[bool] = mapped_column(Boolean, nullable=False, default=True)
    created_at: Mapped[datetime] = mapped_column(
        DateTime(timezone=True), default=lambda: datetime.now(UTC), nullable=False
    )
