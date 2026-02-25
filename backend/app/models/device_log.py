from datetime import UTC, datetime

from sqlalchemy import DateTime, ForeignKey, Integer, String
from sqlalchemy.orm import Mapped, mapped_column

from ..db.base import Base


class DeviceLog(Base):
    __tablename__ = "device_logs"

    id: Mapped[int] = mapped_column(Integer, primary_key=True, autoincrement=True)
    device_id: Mapped[str] = mapped_column(
        String(255), ForeignKey("devices.device_id"), nullable=False, index=True
    )
    room_id: Mapped[str] = mapped_column(
        String(255), ForeignKey("rooms.room_id"), nullable=False, index=True
    )
    source: Mapped[str] = mapped_column(String(32), nullable=False, default="api")
    comp_id: Mapped[str | None] = mapped_column(String(64), nullable=True)
    status: Mapped[int | None] = mapped_column(Integer, nullable=True)
    value: Mapped[int | None] = mapped_column(Integer, nullable=True)
    logged_at: Mapped[datetime] = mapped_column(
        DateTime(timezone=True),
        default=lambda: datetime.now(UTC),
        nullable=False,
        index=True,
    )
