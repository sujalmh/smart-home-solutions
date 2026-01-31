from sqlalchemy import ForeignKey, Integer, String
from sqlalchemy.orm import Mapped, mapped_column

from ..db.base import Base


class SwitchModule(Base):
    __tablename__ = "switch"

    client_id: Mapped[str] = mapped_column(
        String(255),
        ForeignKey("clients.client_id"),
        primary_key=True,
    )
    comp_id: Mapped[str] = mapped_column(String(64), primary_key=True)
    mode: Mapped[int] = mapped_column(Integer, nullable=False, default=1)
    status: Mapped[int] = mapped_column(Integer, nullable=False, default=0)
    value: Mapped[int] = mapped_column(Integer, nullable=False, default=1000)
