from sqlalchemy.ext.asyncio import AsyncEngine

from .base import Base
from .. import models  # noqa: F401
from ..services.room_service import backfill_room_hierarchy
from .session import AsyncSessionLocal


async def init_db(engine: AsyncEngine) -> None:
    async with engine.begin() as conn:
        await conn.run_sync(Base.metadata.create_all)
    async with AsyncSessionLocal() as session:
        await backfill_room_hierarchy(session)
