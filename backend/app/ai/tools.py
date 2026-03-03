from __future__ import annotations

from sqlalchemy import func, select
from sqlalchemy.ext.asyncio import AsyncSession

from ..models.device import Device
from ..models.server import Server
from ..models.switch_module import SwitchModule
from ..services.device_service import send_device_command
from ..websocket.server import is_gateway_connected
from .schemas import DeviceRef, ToolExecutionItem, ToolInput, ToolResult


def _action_to_state(action: str, value: int | None) -> tuple[int, int]:
    if action == "turn_on":
        return 1, value if value is not None else 1000
    if action == "turn_off":
        return 0, value if value is not None else 0
    if action == "set_brightness":
        return 1, value if value is not None else 600
    return 0, 0


async def execute_device_action_tool(
    session: AsyncSession,
    payload: ToolInput,
) -> ToolResult:
    plan = payload.plan
    if plan.room is None:
        return ToolResult(
            status="clarification",
            executed=False,
            reason_codes=["room_required"],
            items=[],
        )
    if not plan.devices:
        return ToolResult(
            status="clarification",
            executed=False,
            reason_codes=["devices_required"],
            items=[],
        )

    if plan.action in {"status_check", "schedule_action", "unknown"}:
        return ToolResult(
            status="no_action",
            executed=False,
            reason_codes=["unsupported_tool_action"],
            items=[],
        )

    stat, val = _action_to_state(plan.action, plan.value)
    val = max(0, min(1000, val))
    items: list[ToolExecutionItem] = []
    any_errors = False

    for device in plan.devices:
        try:
            await send_device_command(
                session=session,
                room_id=plan.room.id,
                device_id=device.id,
                comp=plan.comp,
                mod=1,
                stat=stat,
                val=val,
                source=plan.source,
            )
            items.append(ToolExecutionItem(device=device, status="ok", detail="command_dispatched"))
        except ValueError as error:
            any_errors = True
            items.append(ToolExecutionItem(device=device, status="error", detail=str(error)))

    if any_errors and all(item.status == "error" for item in items):
        return ToolResult(
            status="denied",
            executed=False,
            reason_codes=["all_tool_calls_failed"],
            items=items,
        )
    if any_errors:
        return ToolResult(
            status="executed",
            executed=True,
            reason_codes=["partial_failure"],
            items=items,
        )
    return ToolResult(
        status="executed",
        executed=True,
        reason_codes=[],
        items=items,
    )


def _status_focus_from_message(message: str) -> set[str]:
    text = message.lower()
    focus: set[str] = set()
    if any(token in text for token in ("gateway", "gateways", "server", "servers", "hub", "hubs")):
        focus.add("gateways")
    if any(token in text for token in ("device", "devices", "appliance", "appliances", "client", "clients")):
        focus.add("devices")
    if any(token in text for token in ("switch", "switches", "module", "modules", "comp")):
        focus.add("switches")
    if not focus:
        focus = {"devices", "switches", "gateways"}
    return focus


def _wants_listing(message: str) -> bool:
    text = message.lower()
    return any(
        phrase in text
        for phrase in (
            "which",
            "what devices",
            "what switches",
            "what gateways",
            "list",
            "show",
            "connected devices",
            "online devices",
        )
    )


async def execute_status_query_tool(
    session: AsyncSession,
    payload: ToolInput,
    utterance: str,
) -> ToolResult:
    plan = payload.plan
    focus = _status_focus_from_message(utterance)
    wants_listing = _wants_listing(utterance)
    room = plan.room

    gateways_total = int((await session.scalar(select(func.count()).select_from(Server))) or 0)
    connected_gateways = 0
    if gateways_total > 0:
        server_rows = await session.execute(select(Server.server_id))
        connected_gateways = sum(
            1 for server_id in server_rows.scalars().all() if is_gateway_connected(server_id)
        )

    devices_total = int((await session.scalar(select(func.count()).select_from(Device))) or 0)
    connected_devices = int(
        (
            await session.scalar(
                select(func.count()).select_from(Device).where(
                    Device.server_id.is_not(None),
                    Device.client_id.is_not(None),
                    Device.is_active.is_(True),
                )
            )
        )
        or 0
    )

    switches_total = int((await session.scalar(select(func.count()).select_from(SwitchModule))) or 0)
    switches_on = int(
        (
            await session.scalar(
                select(func.count()).select_from(SwitchModule).where(SwitchModule.status == 1)
            )
        )
        or 0
    )

    room_devices_total = None
    room_connected_devices = None
    room_switches_total = None
    room_switches_on = None
    if room is not None:
        room_devices_total = int(
            (
                await session.scalar(
                    select(func.count()).select_from(Device).where(
                        Device.room_id == room.id,
                        Device.is_active.is_(True),
                    )
                )
            )
            or 0
        )
        room_connected_devices = int(
            (
                await session.scalar(
                    select(func.count()).select_from(Device).where(
                        Device.room_id == room.id,
                        Device.server_id.is_not(None),
                        Device.client_id.is_not(None),
                        Device.is_active.is_(True),
                    )
                )
            )
            or 0
        )
        room_switches_total = int(
            (
                await session.scalar(
                    select(func.count())
                    .select_from(SwitchModule)
                    .join(Device, Device.client_id == SwitchModule.client_id)
                    .where(Device.room_id == room.id)
                )
            )
            or 0
        )
        room_switches_on = int(
            (
                await session.scalar(
                    select(func.count())
                    .select_from(SwitchModule)
                    .join(Device, Device.client_id == SwitchModule.client_id)
                    .where(Device.room_id == room.id, SwitchModule.status == 1)
                )
            )
            or 0
        )

    connected_device_names: list[str] = []
    if wants_listing and "devices" in focus:
        device_stmt = select(Device.name).where(
            Device.server_id.is_not(None),
            Device.client_id.is_not(None),
            Device.is_active.is_(True),
        )
        if room is not None:
            device_stmt = device_stmt.where(Device.room_id == room.id)
        name_rows = await session.execute(device_stmt.order_by(Device.name.asc()).limit(8))
        connected_device_names = list(name_rows.scalars().all())

    switches_on_names: list[str] = []
    if wants_listing and "switches" in focus:
        switch_stmt = (
            select(Device.name, SwitchModule.comp_id)
            .select_from(SwitchModule)
            .join(Device, Device.client_id == SwitchModule.client_id)
            .where(SwitchModule.status == 1)
        )
        if room is not None:
            switch_stmt = switch_stmt.where(Device.room_id == room.id)
        switch_rows = await session.execute(switch_stmt.order_by(Device.name.asc()).limit(8))
        switches_on_names = [f"{device} ({comp})" for device, comp in switch_rows.all()]

    parts: list[str] = []
    if "gateways" in focus:
        parts.append(f"Gateways: {gateways_total} total, {connected_gateways} connected")
    if "devices" in focus:
        if room is not None and room_devices_total is not None and room_connected_devices is not None:
            parts.append(
                f"Devices in {room.name}: {room_devices_total} active, {room_connected_devices} connected"
            )
        else:
            parts.append(f"Devices: {devices_total} total, {connected_devices} connected")
        if wants_listing:
            if connected_device_names:
                parts.append("Connected devices: " + ", ".join(connected_device_names))
            else:
                parts.append("Connected devices: none")
    if "switches" in focus:
        if room is not None and room_switches_total is not None and room_switches_on is not None:
            parts.append(
                f"Switches in {room.name}: {room_switches_total} total, {room_switches_on} on"
            )
        else:
            parts.append(f"Switches: {switches_total} total, {switches_on} on")
        if wants_listing:
            if switches_on_names:
                parts.append("Switches currently on: " + ", ".join(switches_on_names))
            else:
                parts.append("Switches currently on: none")

    summary = ". ".join(parts) + "."
    return ToolResult(
        status="executed",
        executed=True,
        reason_codes=[],
        items=[],
        summary=summary,
    )


def summarize_devices(devices: list[DeviceRef]) -> str:
    return ", ".join(device.name for device in devices)
