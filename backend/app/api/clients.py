from fastapi import APIRouter, Depends, HTTPException, Query
from sqlalchemy import func, select
from sqlalchemy.ext.asyncio import AsyncSession

from ..core.security import get_current_user
from ..db.session import get_async_session
from ..models.client import Client
from ..models.switch_module import SwitchModule
from ..models.user import User
from ..schemas.client import ClientCreate, ClientRead
from ..schemas.switch_module import SwitchModuleRead, SwitchModuleUpdate


from ..websocket.server import is_slave_online


router = APIRouter(prefix="/clients", tags=["clients"])


def _normalize_id(value: str) -> str:
    if value.startswith("RSW-"):
        return value
    return f"RSW-{value}"


def _alt_id(value: str) -> str | None:
    if value.startswith("RSW-"):
        return value[4:]
    return None


@router.get("", response_model=list[ClientRead])
async def list_clients(
    server_id: str | None = Query(default=None),
    session: AsyncSession = Depends(get_async_session),
    _current_user: User = Depends(get_current_user),
) -> list[ClientRead]:
    stmt = select(Client)
    if server_id:
        normalized = _normalize_id(server_id)
        raw = _alt_id(normalized)
        if raw:
            stmt = stmt.where(Client.server_id.in_([normalized, raw]))
        else:
            stmt = stmt.where(Client.server_id == normalized)
    result = await session.execute(stmt)
    clients = list(result.scalars().all())
    if not clients:
        return []

    client_ids = [client.client_id for client in clients]
    count_rows = await session.execute(
        select(SwitchModule.client_id, func.count(SwitchModule.comp_id))
        .where(SwitchModule.client_id.in_(client_ids))
        .group_by(SwitchModule.client_id)
    )
    module_counts: dict[str, int] = {cid: cnt for cid, cnt in count_rows.all()}

    return [
        ClientRead.model_validate(
            {
                "client_id": client.client_id,
                "server_id": client.server_id,
                "ip": client.ip,
                "module_count": module_counts.get(client.client_id, 3),
                "online": is_slave_online(client.server_id, client.client_id),
            }
        )
        for client in clients
    ]


@router.post("", response_model=ClientRead)
async def create_client(
    payload: ClientCreate,
    session: AsyncSession = Depends(get_async_session),
    _current_user: User = Depends(get_current_user),
) -> ClientRead:
    normalized_client_id = _normalize_id(payload.client_id)
    normalized_server_id = _normalize_id(payload.server_id)
    existing = await session.get(Client, normalized_client_id)
    if existing:
        return existing

    client = Client(
        client_id=normalized_client_id,
        server_id=normalized_server_id,
        pwd=payload.pwd,
        ip=payload.ip,
    )
    session.add(client)
    await session.flush()

    count = max(1, min(payload.channel_count, 4))
    for i in range(count):
        session.add(
            SwitchModule(
                client_id=normalized_client_id,
                comp_id=f"Comp{i}",
                mode=1,
                status=0,
                value=1000,
            )
        )

    await session.commit()
    await session.refresh(client)
    return client


@router.get("/{client_id}/modules", response_model=list[SwitchModuleRead])
async def list_modules(
    client_id: str,
    session: AsyncSession = Depends(get_async_session),
    _current_user: User = Depends(get_current_user),
) -> list[SwitchModuleRead]:
    normalized_client_id = _normalize_id(client_id)
    raw_id = _alt_id(normalized_client_id)
    stmt = select(SwitchModule).where(SwitchModule.client_id == normalized_client_id)
    if raw_id:
        stmt = stmt.where(SwitchModule.client_id.in_([normalized_client_id, raw_id]))
    result = await session.execute(stmt)
    return list(result.scalars().all())


@router.put("/{client_id}/modules/{comp_id}", response_model=SwitchModuleRead)
async def update_module(
    client_id: str,
    comp_id: str,
    payload: SwitchModuleUpdate,
    session: AsyncSession = Depends(get_async_session),
    _current_user: User = Depends(get_current_user),
) -> SwitchModuleRead:
    normalized_client_id = _normalize_id(client_id)
    module = await session.get(
        SwitchModule, {"client_id": normalized_client_id, "comp_id": comp_id}
    )
    if module is None:
        raw_id = _alt_id(normalized_client_id)
        if raw_id:
            module = await session.get(
                SwitchModule, {"client_id": raw_id, "comp_id": comp_id}
            )
    if module is None:
        raise HTTPException(
            status_code=404,
            detail=f"Module {comp_id} not found for client {client_id}",
        )
    module.mode = payload.mode
    module.status = payload.status
    module.value = payload.value

    await session.commit()
    await session.refresh(module)
    return module
