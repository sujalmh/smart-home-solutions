from datetime import UTC, datetime

from sqlalchemy import DateTime, ForeignKey, String
from sqlalchemy.orm import Mapped, mapped_column

from ..db.base import Base


class Room(Base):
    __tablename__ = "rooms"

    room_id: Mapped[str] = mapped_column(String(255), primary_key=True)
    home_id: Mapped[str] = mapped_column(
        String(255), ForeignKey("homes.home_id"), nullable=False, index=True
    )
    name: Mapped[str] = mapped_column(String(255), nullable=False)
    created_at: Mapped[datetime] = mapped_column(
        DateTime(timezone=True), default=lambda: datetime.now(UTC), nullable=False
    )
