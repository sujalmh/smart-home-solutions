import re

from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession

from ..models.client import Client
from ..models.device import Device
from ..models.home import Home
from ..models.room import Room
from ..models.server import Server


def _slug(value: str) -> str:
    lowered = value.lower().strip()
    normalized = re.sub(r"[^a-z0-9]+", "-", lowered).strip("-")
    return normalized or "default"


def _display_id(value: str) -> str:
    if value.startswith("RSW-"):
        return value[4:]
    return value


def _home_id_from_server(server_id: str) -> str:
    return f"home-{_slug(_display_id(server_id))}"


def _room_id_from_home(home_id: str) -> str:
    return f"{home_id}-default-room"


def _device_id_from_client(client_id: str) -> str:
    return f"device-{_slug(_display_id(client_id))}"


async def backfill_room_hierarchy(session: AsyncSession) -> None:
    servers = list((await session.execute(select(Server))).scalars().all())
    clients = list((await session.execute(select(Client))).scalars().all())

    existing_homes = {
        home.home_id: home
        for home in (await session.execute(select(Home))).scalars().all()
    }
    existing_rooms = {
        room.room_id: room
        for room in (await session.execute(select(Room))).scalars().all()
    }
    existing_devices = {
        device.client_id: device
        for device in (await session.execute(select(Device))).scalars().all()
        if device.client_id
    }

    for server in servers:
        home_id = _home_id_from_server(server.server_id)
        if home_id not in existing_homes:
            home = Home(home_id=home_id, name=f"Home {_display_id(server.server_id)}")
            session.add(home)
            existing_homes[home_id] = home

        room_id = _room_id_from_home(home_id)
        if room_id not in existing_rooms:
            room = Room(room_id=room_id, home_id=home_id, name="Default Room")
            session.add(room)
            existing_rooms[room_id] = room

    for client in clients:
        if client.client_id in existing_devices:
            continue
        home_id = _home_id_from_server(client.server_id)
        room_id = _room_id_from_home(home_id)
        if home_id not in existing_homes:
            home = Home(home_id=home_id, name=f"Home {_display_id(client.server_id)}")
            session.add(home)
            existing_homes[home_id] = home
        if room_id not in existing_rooms:
            room = Room(room_id=room_id, home_id=home_id, name="Default Room")
            session.add(room)
            existing_rooms[room_id] = room

        session.add(
            Device(
                device_id=_device_id_from_client(client.client_id),
                room_id=room_id,
                server_id=client.server_id,
                client_id=client.client_id,
                name=f"Device {_display_id(client.client_id)}",
                device_type="switch",
                is_active=True,
            )
        )

    await session.commit()
