from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession

from ..db.session import get_async_session
from ..models.client import Client
from ..models.server import Server
from ..models.switch_module import SwitchModule
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
from ..schemas.switch_module import SwitchModuleRead
from ..websocket.server import (
    emit_gateway_bind,
    emit_gateway_command,
    emit_gateway_status,
    is_gateway_connected,
)


router = APIRouter(prefix="/devices", tags=["devices"])


def _default_server_pwd(server_id: str) -> str:
    if server_id.startswith("RSW-"):
        return server_id[4:]
    return server_id


@router.post("/{server_id}/command", response_model=DeviceCommandResponse)
async def device_command(
    server_id: str,
    payload: DeviceCommandRequest,
    session: AsyncSession = Depends(get_async_session),
) -> DeviceCommandResponse:
    server = await session.get(Server, server_id)
    if server is None:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="server_not_found")

    client = await session.get(Client, payload.dev_id)
    if client is None:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="client_not_found")

    await emit_gateway_command(
        payload.server_id,
        payload.dev_id,
        payload.comp,
        payload.mod,
        payload.stat,
        payload.val,
    )
    return DeviceCommandResponse.model_validate(payload.model_dump())


@router.post("/{server_id}/status", response_model=list[SwitchModuleRead])
async def device_status(
    server_id: str,
    payload: DeviceStatusRequest,
    session: AsyncSession = Depends(get_async_session),
) -> list[SwitchModuleRead]:
    server = await session.get(Server, server_id)
    if server is None:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="server_not_found")

    client = await session.get(Client, payload.dev_id)
    if client is None:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="client_not_found")

    await emit_gateway_status(payload.server_id, payload.dev_id, payload.comp)

    stmt = select(SwitchModule).where(SwitchModule.client_id == payload.dev_id)
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
) -> DeviceConfigResponse:
    existing = await session.get(Server, server_id)
    if existing is None:
        server = Server(
            server_id=server_id,
            pwd=(payload.pwd if payload and payload.pwd else _default_server_pwd(server_id)),
            ip=(payload.ip if payload and payload.ip else "192.168.4.100"),
        )
        session.add(server)
        await session.commit()
    return DeviceConfigResponse(status="ok")


@router.post("/{server_id}/config/remote", response_model=DeviceConfigResponse)
async def config_remote(
    server_id: str,
    payload: DeviceRemoteConfigRequest,
    session: AsyncSession = Depends(get_async_session),
) -> DeviceConfigResponse:
    server = await session.get(Server, server_id)
    if server is None:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="server_not_found")
    return DeviceConfigResponse(status="ok")


@router.post("/{server_id}/config/client", response_model=DeviceConfigResponse)
async def config_client(
    server_id: str,
    payload: DeviceClientConfigRequest,
    session: AsyncSession = Depends(get_async_session),
) -> DeviceConfigResponse:
    server = await session.get(Server, server_id)
    if server is None:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="server_not_found")

    existing = await session.get(Client, payload.client_id)
    if existing is None:
        client = Client(
            client_id=payload.client_id,
            server_id=server_id,
            pwd=payload.pwd,
            ip=payload.ip,
        )
        session.add(client)
        await session.flush()

        default_modules = ["Comp0", "Comp1", "Comp2"]
        for comp_id in default_modules:
            session.add(
                SwitchModule(
                    client_id=payload.client_id,
                    comp_id=comp_id,
                    mode=1,
                    status=0,
                    value=1000,
                )
            )

        await session.commit()

    return DeviceConfigResponse(status="ok")


@router.post("/{server_id}/register", response_model=DeviceConfigResponse)
async def register_device(
    server_id: str,
    payload: DeviceRegisterRequest | None = None,
    session: AsyncSession = Depends(get_async_session),
) -> DeviceConfigResponse:
    existing = await session.get(Server, server_id)
    if existing is None:
        server = Server(
            server_id=server_id,
            pwd=_default_server_pwd(server_id),
            ip="192.168.4.100",
        )
        session.add(server)
        await session.commit()

    return DeviceConfigResponse(status="ok")


@router.get("/{server_id}/gateway/status")
async def gateway_status(server_id: str) -> dict:
    return {"online": is_gateway_connected(server_id)}


@router.post("/{server_id}/gateway/bind", response_model=DeviceConfigResponse)
async def gateway_bind(
    server_id: str,
    payload: DeviceBindRequest,
    session: AsyncSession = Depends(get_async_session),
) -> DeviceConfigResponse:
    server = await session.get(Server, server_id)
    if server is None:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="server_not_found")

    client = await session.get(Client, payload.client_id)
    if client is None:
        client = Client(
            client_id=payload.client_id,
            server_id=server_id,
            pwd=payload.client_id,
            ip="0.0.0.0",
        )
        session.add(client)
        await session.commit()

    await emit_gateway_bind(server_id, payload.client_id)
    return DeviceConfigResponse(status="ok")
