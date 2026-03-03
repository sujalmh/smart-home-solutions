import asyncio
import time

from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy import delete, func, select
from sqlalchemy.ext.asyncio import AsyncSession

from ..core.security import get_current_user
from ..db.session import get_async_session
from ..models.client import Client
from ..models.server import Server
from ..models.switch_module import SwitchModule
from ..models.user import User
from ..schemas.device import (
    DeviceBindRequest,
    DeviceClientConfigRequest,
    DeviceCommandRequest,
    DeviceCommandResponse,
    DeviceConfigResponse,
    DeviceRegisterRequest,
    DeviceRemoteConfigRequest,
    DeviceServerConfigRequest,
    DeviceStatusRequest,
)
from ..schemas.client import ClientRead
from ..schemas.switch_module import SwitchModuleRead
from ..websocket.server import (
    emit_gateway_bind,
    emit_gateway_command,
    emit_gateway_status,
    emit_gateway_unbind,
    is_gateway_connected,
    list_seen_slaves,
)


router = APIRouter(prefix="/devices", tags=["devices"])
_status_refresh_cooldown_s = 1.0
_last_status_refresh_at: dict[tuple[str, str, str], float] = {}


def _default_server_pwd(server_id: str) -> str:
    if server_id.startswith("RSW-"):
        return server_id[4:]
    return server_id


def _normalize_id(value: str) -> str:
    if value.startswith("RSW-"):
        return value
    return f"RSW-{value}"


def _module_count_from_device_type(device_type: str | None) -> int:
    if not device_type:
        return 4
    normalized = device_type.strip().lower()
    if "4ch" in normalized or "4-channel" in normalized or "multi" in normalized:
        return 4
    if (
        "1ch" in normalized
        or "1-channel" in normalized
        or "1channel" in normalized
        or normalized == "switch"
        or normalized == "single"
    ):
        return 1
    return 4


async def _get_client_any(session: AsyncSession, client_id: str) -> Client | None:
    normalized = _normalize_id(client_id)
    raw = client_id[4:] if client_id.startswith("RSW-") else client_id
    client = await session.get(Client, normalized)
    if client is None and raw != normalized:
        client = await session.get(Client, raw)
    return client


async def _seed_default_modules(session: AsyncSession, client_id: str, count: int = 4) -> None:
    all_comps = ["Comp0", "Comp1", "Comp2", "Comp3"]
    # Only seed the comps needed for this channel count
    default_modules = all_comps[:max(1, min(count, 4))]
    result = await session.execute(
        select(SwitchModule.comp_id).where(SwitchModule.client_id == client_id)
    )
    existing = {row[0] for row in result.all()}
    for comp_id in default_modules:
        if comp_id in existing:
            continue
        session.add(
            SwitchModule(
                client_id=client_id,
                comp_id=comp_id,
                mode=1,
                status=0,
                value=1000,
            )
        )


@router.post("/{server_id}/command", response_model=DeviceCommandResponse)
async def device_command(
    server_id: str,
    payload: DeviceCommandRequest,
    session: AsyncSession = Depends(get_async_session),
    _current_user: User = Depends(get_current_user),
) -> DeviceCommandResponse:
    normalized_server_id = _normalize_id(server_id)
    server = await session.get(Server, normalized_server_id)
    if server is None:
        server = Server(
            server_id=normalized_server_id,
            pwd=_default_server_pwd(normalized_server_id),
            ip="",
        )
        session.add(server)
        await session.commit()

    client = await _get_client_any(session, payload.dev_id)
    if client is None:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="client_not_found")
    normalized_dev_id = _normalize_id(payload.dev_id)

    if client.server_id not in {normalized_server_id, server_id}:
        raise HTTPException(status_code=status.HTTP_409_CONFLICT, detail="device_unbound")

    if not is_gateway_connected(normalized_server_id):
        raise HTTPException(status_code=status.HTTP_409_CONFLICT, detail="gateway_offline")

    delivered = await emit_gateway_command(
        normalized_server_id,
        normalized_dev_id,
        payload.comp,
        payload.mod,
        payload.stat,
        payload.val,
        payload.req_id,
    )
    if not delivered:
        raise HTTPException(status_code=status.HTTP_409_CONFLICT, detail="gateway_offline")
    return DeviceCommandResponse.model_validate(
        {
            "serverID": normalized_server_id,
            "devID": normalized_dev_id,
            "comp": payload.comp,
            "mod": payload.mod,
            "stat": payload.stat,
            "val": payload.val,
            "reqId": payload.req_id,
        }
    )


@router.post("/{server_id}/status", response_model=list[SwitchModuleRead])
async def device_status(
    server_id: str,
    payload: DeviceStatusRequest,
    session: AsyncSession = Depends(get_async_session),
    _current_user: User = Depends(get_current_user),
) -> list[SwitchModuleRead]:
    normalized_server_id = _normalize_id(server_id)
    server = await session.get(Server, normalized_server_id)
    if server is None:
        server = Server(
            server_id=normalized_server_id,
            pwd=_default_server_pwd(normalized_server_id),
            ip="",
        )
        session.add(server)
        await session.commit()

    client = await _get_client_any(session, payload.dev_id)
    if client is None:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="client_not_found")
    normalized_dev_id = _normalize_id(payload.dev_id)

    if payload.refresh:
        cooldown_key = (
            normalized_server_id,
            normalized_dev_id,
            payload.comp if payload.comp else "*",
        )
        now = time.monotonic()
        last = _last_status_refresh_at.get(cooldown_key)
        should_refresh_gateway = (
            last is None or now - last >= _status_refresh_cooldown_s
        )

        if should_refresh_gateway:
            if not is_gateway_connected(normalized_server_id):
                raise HTTPException(
                    status_code=status.HTTP_409_CONFLICT, detail="gateway_offline"
                )

            delivered = await emit_gateway_status(
                normalized_server_id, normalized_dev_id, payload.comp
            )
            if not delivered:
                raise HTTPException(
                    status_code=status.HTTP_409_CONFLICT, detail="gateway_offline"
                )

            _last_status_refresh_at[cooldown_key] = now
            await asyncio.sleep(0.15)

    stmt = select(SwitchModule).where(SwitchModule.client_id == normalized_dev_id)
    if payload.comp:
        stmt = stmt.where(SwitchModule.comp_id == payload.comp)
    result = await session.execute(stmt)
    modules = list(result.scalars().all())
    return [SwitchModuleRead.model_validate(module) for module in modules]


@router.post("/{server_id}/config/server", response_model=DeviceConfigResponse)
async def config_server(
    server_id: str,
    payload: DeviceServerConfigRequest | None = None,
    session: AsyncSession = Depends(get_async_session),
    _current_user: User = Depends(get_current_user),
) -> DeviceConfigResponse:
    normalized_server_id = _normalize_id(server_id)
    existing = await session.get(Server, normalized_server_id)
    if existing is None:
        server = Server(
            server_id=normalized_server_id,
            pwd=(
                payload.pwd
                if payload and payload.pwd
                else _default_server_pwd(normalized_server_id)
            ),
            ip=(payload.ip if payload and payload.ip else ""),
        )
        session.add(server)
        await session.commit()
    return DeviceConfigResponse(status="ok")


@router.post("/{server_id}/config/remote", response_model=DeviceConfigResponse)
async def config_remote(
    server_id: str,
    payload: DeviceRemoteConfigRequest,
    session: AsyncSession = Depends(get_async_session),
    _current_user: User = Depends(get_current_user),
) -> DeviceConfigResponse:
    normalized_server_id = _normalize_id(server_id)
    server = await session.get(Server, normalized_server_id)
    if server is None:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="server_not_found")
    return DeviceConfigResponse(status="ok")


@router.post("/{server_id}/config/client", response_model=DeviceConfigResponse)
async def config_client(
    server_id: str,
    payload: DeviceClientConfigRequest,
    session: AsyncSession = Depends(get_async_session),
    _current_user: User = Depends(get_current_user),
) -> DeviceConfigResponse:
    normalized_server_id = _normalize_id(server_id)
    normalized_client_id = _normalize_id(payload.client_id)
    server = await session.get(Server, normalized_server_id)
    if server is None:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="server_not_found")

    existing = await session.get(Client, normalized_client_id)
    if existing is None:
        client = Client(
            client_id=normalized_client_id,
            server_id=normalized_server_id,
            pwd=payload.pwd,
            ip=payload.ip,
        )
        session.add(client)
        await session.flush()

        await _seed_default_modules(session, normalized_client_id)
        await session.commit()

    return DeviceConfigResponse(status="ok")


@router.post("/{server_id}/register", response_model=DeviceConfigResponse)
async def register_device(
    server_id: str,
    payload: DeviceRegisterRequest | None = None,
    session: AsyncSession = Depends(get_async_session),
    _current_user: User = Depends(get_current_user),
) -> DeviceConfigResponse:
    normalized_server_id = _normalize_id(server_id)
    existing = await session.get(Server, normalized_server_id)
    if existing is None:
        server = Server(
            server_id=normalized_server_id,
            pwd=_default_server_pwd(normalized_server_id),
            ip="",
        )
        session.add(server)
        await session.commit()

    return DeviceConfigResponse(status="ok")


@router.get("/{server_id}/gateway/status")
async def gateway_status(
    server_id: str,
    _current_user: User = Depends(get_current_user),
) -> dict:
    normalized_server_id = _normalize_id(server_id)
    return {"online": is_gateway_connected(normalized_server_id)}


@router.get("/{server_id}/gateway/seen")
async def gateway_seen(
    server_id: str,
    session: AsyncSession = Depends(get_async_session),
    _current_user: User = Depends(get_current_user),
) -> list[dict[str, str]]:
    normalized_server_id = _normalize_id(server_id)
    seen = await list_seen_slaves(normalized_server_id)
    # Exclude slaves bound to ANY server, not just the requesting one
    result = await session.execute(select(Client.client_id))
    bound_wire = {
        client_id[4:] if client_id.startswith("RSW-") else client_id
        for (client_id,) in result.all()
    }
    return [entry for entry in seen if entry.get("clientID") not in bound_wire]


@router.get("/{server_id}/bound", response_model=list[ClientRead])
async def bound_devices(
    server_id: str,
    session: AsyncSession = Depends(get_async_session),
    _current_user: User = Depends(get_current_user),
) -> list[ClientRead]:
    normalized_server_id = _normalize_id(server_id)
    result = await session.execute(
        select(Client).where(Client.server_id.in_([normalized_server_id, server_id]))
    )
    clients = list(result.scalars().all())
    if not clients:
        return []

    client_ids = [client.client_id for client in clients]

    # Derive module_count from the number of seeded SwitchModule rows.
    # This is reliable: bind always seeds exactly as many comps as the device
    # channel count, so the row count is the authoritative source of truth.
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
                "module_count": module_counts.get(client.client_id, 4),
            }
        )
        for client in clients
    ]


@router.post("/{server_id}/gateway/bind", response_model=DeviceConfigResponse)
async def gateway_bind(
    server_id: str,
    payload: DeviceBindRequest,
    session: AsyncSession = Depends(get_async_session),
    _current_user: User = Depends(get_current_user),
) -> DeviceConfigResponse:
    normalized_server_id = _normalize_id(server_id)
    normalized_client_id = _normalize_id(payload.client_id)
    server = await session.get(Server, normalized_server_id)
    if server is None:
        server = Server(
            server_id=normalized_server_id,
            pwd=_default_server_pwd(normalized_server_id),
            ip="",
        )
        session.add(server)
        await session.commit()

    client = await _get_client_any(session, payload.client_id)
    bound_client_id = normalized_client_id
    if client is None:
        client = Client(
            client_id=normalized_client_id,
            server_id=normalized_server_id,
            pwd=normalized_client_id,
            ip="",
        )
        session.add(client)
        await session.flush()
    else:
        # Reject if already bound to a different server
        if client.server_id not in (normalized_server_id, server_id, "", None):
            raise HTTPException(
                status_code=status.HTTP_409_CONFLICT,
                detail=f"Device already bound to server {client.server_id}",
            )
        client.server_id = normalized_server_id
        bound_client_id = client.client_id

    # Enforce exact channel layout on every bind/rebind.
    await session.execute(
        delete(SwitchModule).where(SwitchModule.client_id == bound_client_id)
    )
    await _seed_default_modules(session, bound_client_id, payload.channel_count)
    await session.commit()

    await emit_gateway_bind(normalized_server_id, normalized_client_id)
    return DeviceConfigResponse(status="ok")


@router.post("/{server_id}/gateway/unbind", response_model=DeviceConfigResponse)
async def gateway_unbind(
    server_id: str,
    payload: DeviceBindRequest,
    session: AsyncSession = Depends(get_async_session),
    _current_user: User = Depends(get_current_user),
) -> DeviceConfigResponse:
    normalized_server_id = _normalize_id(server_id)
    normalized_client_id = _normalize_id(payload.client_id)
    server = await session.get(Server, normalized_server_id)
    if server is None:
        server = Server(
            server_id=normalized_server_id,
            pwd=_default_server_pwd(normalized_server_id),
            ip="",
        )
        session.add(server)
        await session.commit()

    await emit_gateway_unbind(normalized_server_id, normalized_client_id)
    await session.execute(
        delete(SwitchModule).where(SwitchModule.client_id == normalized_client_id)
    )
    await session.execute(delete(Client).where(Client.client_id == normalized_client_id))
    await session.commit()
    return DeviceConfigResponse(status="ok")
