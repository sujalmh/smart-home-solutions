from sqlalchemy import String
from sqlalchemy.orm import Mapped, mapped_column

from ..db.base import Base


class User(Base):
    __tablename__ = "users"

    email_id: Mapped[str] = mapped_column(String(255), primary_key=True)
    pwd: Mapped[str] = mapped_column(String(255), nullable=False)
    device_id: Mapped[str] = mapped_column(String(255), nullable=False)
