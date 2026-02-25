from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession

from ..core.security import get_current_user
from ..db.session import get_async_session
from ..models.device import Device
from ..models.home import Home
from ..models.room import Room
from ..models.user import User
from ..schemas.room import (
    HomeRead,
    RoomDeviceCommandRequest,
    RoomDeviceCommandResponse,
    RoomDeviceRead,
    RoomRead,
)
from ..services.device_service import send_device_command


router = APIRouter(tags=["rooms"])


@router.get("/homes", response_model=list[HomeRead])
async def list_homes(
    session: AsyncSession = Depends(get_async_session),
    _current_user: User = Depends(get_current_user),
) -> list[HomeRead]:
    result = await session.execute(select(Home).order_by(Home.home_id))
    return list(result.scalars().all())


@router.get("/homes/{home_id}/rooms", response_model=list[RoomRead])
async def list_rooms(
    home_id: str,
    session: AsyncSession = Depends(get_async_session),
    _current_user: User = Depends(get_current_user),
) -> list[RoomRead]:
    result = await session.execute(
        select(Room).where(Room.home_id == home_id).order_by(Room.room_id)
    )
    return list(result.scalars().all())


@router.get("/rooms/{room_id}/devices", response_model=list[RoomDeviceRead])
async def list_room_devices(
    room_id: str,
    session: AsyncSession = Depends(get_async_session),
    _current_user: User = Depends(get_current_user),
) -> list[RoomDeviceRead]:
    result = await session.execute(
        select(Device).where(Device.room_id == room_id).order_by(Device.device_id)
    )
    return list(result.scalars().all())


@router.post(
    "/rooms/{room_id}/devices/{device_id}/commands",
    response_model=RoomDeviceCommandResponse,
)
async def command_room_device(
    room_id: str,
    device_id: str,
    payload: RoomDeviceCommandRequest,
    session: AsyncSession = Depends(get_async_session),
    _current_user: User = Depends(get_current_user),
) -> RoomDeviceCommandResponse:
    try:
        command = await send_device_command(
            session=session,
            room_id=room_id,
            device_id=device_id,
            comp=payload.comp,
            mod=payload.mod,
            stat=payload.stat,
            val=payload.val,
            source=payload.source,
        )
    except ValueError as error:
        if str(error) == "device_not_found":
            raise HTTPException(
                status_code=status.HTTP_404_NOT_FOUND,
                detail="device_not_found",
            ) from error
        raise
    return RoomDeviceCommandResponse.model_validate(command)
