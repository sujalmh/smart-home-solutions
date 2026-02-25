from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession

from ..models.device import Device
from ..models.device_log import DeviceLog
from ..models.switch_module import SwitchModule
from ..websocket.server import emit_gateway_command


def _normalize_id(value: str) -> str:
    if value.startswith("RSW-"):
        return value
    return f"RSW-{value}"


async def send_device_command(
    session: AsyncSession,
    room_id: str,
    device_id: str,
    comp: str,
    mod: int,
    stat: int,
    val: int,
    source: str,
) -> dict[str, str | int]:
    result = await session.execute(
        select(Device).where(Device.room_id == room_id, Device.device_id == device_id)
    )
    device = result.scalar_one_or_none()
    if device is None or not device.client_id or not device.server_id:
        raise ValueError("device_not_found")

    server_id = _normalize_id(device.server_id)
    dev_id = _normalize_id(device.client_id)

    await emit_gateway_command(server_id, dev_id, comp, mod, stat, val)

    module = await session.get(SwitchModule, {"client_id": dev_id, "comp_id": comp})
    if module is None:
        module = SwitchModule(
            client_id=dev_id,
            comp_id=comp,
            mode=mod,
            status=stat,
            value=val,
        )
        session.add(module)
    else:
        module.mode = mod
        module.status = stat
        module.value = val

    session.add(
        DeviceLog(
            device_id=device_id,
            room_id=room_id,
            source=source,
            comp_id=comp,
            status=stat,
            value=val,
        )
    )
    await session.commit()

    return {
        "room_id": room_id,
        "device_id": device_id,
        "server_id": server_id,
        "dev_id": dev_id,
        "comp": comp,
        "mod": mod,
        "stat": stat,
        "val": val,
    }
