from __future__ import annotations

from pathlib import Path

import pytest
import pytest_asyncio
from sqlalchemy.ext.asyncio import AsyncSession, async_sessionmaker, create_async_engine

from app.ai.orchestrator import AIOrchestrator
from app.ai.schemas import (
    ExecutionPlan,
    Intent,
    NLUHint,
    PersistedState,
    ResolvedEntities,
    RoomRef,
    ToolInput,
)
from app.ai.tools import execute_device_action_tool
from app.db.base import Base
from app.models.device import Device
from app.models.home import Home
from app.models.room import Room


@pytest_asyncio.fixture
async def session(tmp_path: Path) -> AsyncSession:
    db_path = tmp_path / "ai_invariants.db"
    engine = create_async_engine(f"sqlite+aiosqlite:///{db_path}")
    session_maker = async_sessionmaker(engine, expire_on_commit=False)

    async with engine.begin() as conn:
        await conn.run_sync(Base.metadata.create_all)

    async with session_maker() as db_session:
        db_session.add(Home(home_id="home-1", name="Home 1"))
        db_session.add(Room(room_id="room-1", home_id="home-1", name="Living Room"))
        db_session.add(
            Device(
                device_id="device-1",
                room_id="room-1",
                server_id=None,
                client_id=None,
                name="Ceiling Light",
                device_type="light",
                is_active=True,
            )
        )
        db_session.add(
            Device(
                device_id="device-2",
                room_id="room-1",
                server_id=None,
                client_id=None,
                name="Wall Light",
                device_type="light",
                is_active=True,
            )
        )
        await db_session.commit()
        yield db_session

    await engine.dispose()


@pytest.mark.asyncio
async def test_resolver_expands_to_all_devices_in_room(session: AsyncSession) -> None:
    orchestrator = AIOrchestrator()
    state = {
        "session": session,
        "intent": Intent(action="turn_off", confidence=0.95),
        "hint": NLUHint(room="Living Room", device=None, value=None, comp=None),
        "persisted_state": PersistedState(),
    }

    output = await orchestrator._resolver(state)  # noqa: SLF001
    entities = ResolvedEntities.model_validate(output["entities"])

    assert entities.room is not None
    assert entities.room.id == "room-1"
    assert sorted(device.id for device in entities.devices) == ["device-1", "device-2"]


@pytest.mark.asyncio
async def test_planner_outputs_structured_plan() -> None:
    orchestrator = AIOrchestrator()
    entities = ResolvedEntities(
        room=RoomRef(id="room-1", name="Living Room"),
        devices=[],
        action="status_check",
        value=None,
        comp="Comp1",
    )
    state = {
        "request": type("Req", (), {"confirm": None})(),
        "pending_plan": None,
        "entities": entities,
    }

    output = await orchestrator._planner(state)  # noqa: SLF001
    plan = ExecutionPlan.model_validate(output["plan"])

    assert plan.action == "status_check"
    assert plan.room is not None
    assert plan.room.id == "room-1"
    assert plan.comp == "Comp1"


@pytest.mark.asyncio
async def test_safety_requires_confirmation_for_schedule() -> None:
    orchestrator = AIOrchestrator()
    plan = ExecutionPlan(
        action="schedule_action",
        room=RoomRef(id="room-1", name="Living Room"),
        devices=[],
        value=None,
        comp="Comp0",
        source="ai",
    )
    state = {
        "request": type("Req", (), {"confirm": None, "conversation_id": "default"})(),
        "intent": Intent(action="schedule_action", confidence=0.9),
        "plan": plan,
        "pending_plan": None,
    }

    output = await orchestrator._safety(state)  # noqa: SLF001
    decision = output["decision"]
    assert decision.decision == "confirm"
    assert "high_risk_schedule_requires_confirmation" in decision.reason_codes


@pytest.mark.asyncio
async def test_tool_returns_clarification_for_missing_targets(session: AsyncSession) -> None:
    plan = ExecutionPlan(
        action="turn_off",
        room=None,
        devices=[],
        value=None,
        comp="Comp0",
        source="ai",
    )
    result = await execute_device_action_tool(session, ToolInput(plan=plan))

    assert result.status == "clarification"
    assert result.executed is False
    assert "room_required" in result.reason_codes
