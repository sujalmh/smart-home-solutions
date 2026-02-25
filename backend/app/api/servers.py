from fastapi import APIRouter, Depends
from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession

from ..core.security import get_current_user
from ..db.session import get_async_session
from ..models.server import Server
from ..models.user import User
from ..schemas.server import ServerCreate, ServerRead


router = APIRouter(prefix="/servers", tags=["servers"])


@router.get("", response_model=list[ServerRead])
async def list_servers(
    session: AsyncSession = Depends(get_async_session),
    _current_user: User = Depends(get_current_user),
) -> list[ServerRead]:
    result = await session.execute(select(Server))
    return list(result.scalars().all())


@router.post("", response_model=ServerRead)
async def create_server(
    payload: ServerCreate,
    session: AsyncSession = Depends(get_async_session),
    _current_user: User = Depends(get_current_user),
) -> ServerRead:
    existing = await session.get(Server, payload.server_id)
    if existing:
        return existing
    server = Server(server_id=payload.server_id, pwd=payload.pwd, ip=payload.ip)
    session.add(server)
    await session.commit()
    await session.refresh(server)
    return server
