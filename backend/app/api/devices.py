from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession

from ..db.session import get_async_session
from ..models.client import Client
from ..models.server import Server
from ..models.switch_module import SwitchModule
from ..schemas.device import (
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

    module = await session.get(
        SwitchModule, {"client_id": payload.dev_id, "comp_id": payload.comp}
    )
    if module is None:
        module = SwitchModule(
            client_id=payload.dev_id,
            comp_id=payload.comp,
            mode=payload.mod,
            status=payload.stat,
            value=payload.val,
        )
        session.add(module)
    else:
        module.mode = payload.mod
        module.status = payload.stat
        module.value = payload.val

    await session.commit()
    return payload


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

    stmt = select(SwitchModule).where(SwitchModule.client_id == payload.dev_id)
    if payload.comp:
        stmt = stmt.where(SwitchModule.comp_id == payload.comp)
    result = await session.execute(stmt)
    return list(result.scalars().all())


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
