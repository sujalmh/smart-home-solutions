from fastapi import APIRouter, Depends, Query
from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession

from ..db.session import get_async_session
from ..models.client import Client
from ..models.switch_module import SwitchModule
from ..schemas.client import ClientCreate, ClientRead
from ..schemas.switch_module import SwitchModuleRead, SwitchModuleUpdate


router = APIRouter(prefix="/clients", tags=["clients"])


def _normalize_id(value: str) -> str:
    if value.startswith("RSW-"):
        return value
    return f"RSW-{value}"


@router.get("", response_model=list[ClientRead])
async def list_clients(
    server_id: str | None = Query(default=None),
    session: AsyncSession = Depends(get_async_session),
) -> list[ClientRead]:
    stmt = select(Client)
    if server_id:
        stmt = stmt.where(Client.server_id == _normalize_id(server_id))
    result = await session.execute(stmt)
    return list(result.scalars().all())


@router.post("", response_model=ClientRead)
async def create_client(
    payload: ClientCreate,
    session: AsyncSession = Depends(get_async_session),
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

    default_modules = ["Comp0", "Comp1", "Comp2", "Comp3"]
    for comp_id in default_modules:
        session.add(
            SwitchModule(
                client_id=normalized_client_id,
                comp_id=comp_id,
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
) -> list[SwitchModuleRead]:
    normalized_client_id = _normalize_id(client_id)
    result = await session.execute(
        select(SwitchModule).where(SwitchModule.client_id == normalized_client_id)
    )
    return list(result.scalars().all())


@router.put("/{client_id}/modules/{comp_id}", response_model=SwitchModuleRead)
async def update_module(
    client_id: str,
    comp_id: str,
    payload: SwitchModuleUpdate,
    session: AsyncSession = Depends(get_async_session),
) -> SwitchModuleRead:
    normalized_client_id = _normalize_id(client_id)
    module = await session.get(
        SwitchModule, {"client_id": normalized_client_id, "comp_id": comp_id}
    )
    if module is None:
        module = SwitchModule(
            client_id=client_id,
            comp_id=comp_id,
            mode=payload.mode,
            status=payload.status,
            value=payload.value,
        )
        session.add(module)
    else:
        module.mode = payload.mode
        module.status = payload.status
        module.value = payload.value

    await session.commit()
    await session.refresh(module)
    return module
