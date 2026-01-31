from sqlalchemy import String
from sqlalchemy.orm import Mapped, mapped_column

from ..db.base import Base


class Server(Base):
    __tablename__ = "servers"

    server_id: Mapped[str] = mapped_column(String(255), primary_key=True)
    pwd: Mapped[str] = mapped_column(String(255), nullable=False)
    ip: Mapped[str] = mapped_column(String(64), nullable=False)
