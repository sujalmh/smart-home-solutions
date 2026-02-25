from __future__ import annotations

from sqlalchemy.ext.asyncio import AsyncSession

from ..services.device_service import send_device_command
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


def summarize_devices(devices: list[DeviceRef]) -> str:
    return ", ".join(device.name for device in devices)
