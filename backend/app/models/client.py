from sqlalchemy import ForeignKey, String
from sqlalchemy.orm import Mapped, mapped_column

from ..db.base import Base


class Client(Base):
    __tablename__ = "clients"

    client_id: Mapped[str] = mapped_column(String(255), primary_key=True)
    server_id: Mapped[str] = mapped_column(
        String(255),
        ForeignKey("servers.server_id"),
        nullable=False,
        index=True,
    )
    pwd: Mapped[str] = mapped_column(String(255), nullable=False)
    ip: Mapped[str] = mapped_column(String(64), nullable=False)
