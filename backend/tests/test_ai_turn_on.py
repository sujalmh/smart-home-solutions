"""End-to-end tests for AI "turn on" device commands.

Validates that when the user says "Turn on the living room" (without an LLM),
the NLU → resolver → planner → safety → tool pipeline correctly dispatches
commands to every device in that room and returns an "executed" response.
"""

from __future__ import annotations

from pathlib import Path
from unittest.mock import AsyncMock, patch

import pytest
import pytest_asyncio
from sqlalchemy.ext.asyncio import AsyncSession, async_sessionmaker, create_async_engine

from app.ai.orchestrator import AIOrchestrator
from app.ai.schemas import AIChatRequest
from app.db.base import Base
from app.models.client import Client
from app.models.device import Device
from app.models.home import Home
from app.models.room import Room
from app.models.server import Server
from app.models.switch_module import SwitchModule


# ---------------------------------------------------------------------------
# Fixture: in-memory SQLite with a fully wired room
# (Home → Room → Server → Client → Device with server_id/client_id)
# ---------------------------------------------------------------------------

@pytest_asyncio.fixture
async def session(tmp_path: Path) -> AsyncSession:
    db_path = tmp_path / "ai_turn_on.db"
    engine = create_async_engine(f"sqlite+aiosqlite:///{db_path}")
    session_maker = async_sessionmaker(engine, expire_on_commit=False)

    async with engine.begin() as conn:
        await conn.run_sync(Base.metadata.create_all)

    async with session_maker() as db_session:
        db_session.add(Home(home_id="home-1", name="Home 1"))
        db_session.add(Room(room_id="room-1", home_id="home-1", name="Living Room"))

        db_session.add(Server(server_id="RSW-1001", pwd="1001", ip="192.168.0.10"))
        db_session.add(Client(client_id="RSW-2001", server_id="RSW-1001", pwd="2001", ip=""))
        db_session.add(Client(client_id="RSW-2002", server_id="RSW-1001", pwd="2002", ip=""))

        db_session.add(
            Device(
                device_id="device-1",
                room_id="room-1",
                server_id="RSW-1001",
                client_id="RSW-2001",
                name="Ceiling Light",
                device_type="light",
                is_active=True,
            )
        )
        db_session.add(
            Device(
                device_id="device-2",
                room_id="room-1",
                server_id="RSW-1001",
                client_id="RSW-2002",
                name="Wall Light",
                device_type="light",
                is_active=True,
            )
        )

        # Pre-existing switch module (OFF)
        db_session.add(
            SwitchModule(
                client_id="RSW-2001",
                comp_id="Comp0",
                mode=1,
                status=0,
                value=0,
            )
        )
        await db_session.commit()
        yield db_session

    await engine.dispose()


# ---------------------------------------------------------------------------
# Helper: create an orchestrator with LLM disabled
# ---------------------------------------------------------------------------

def _make_orchestrator() -> AIOrchestrator:
    orch = AIOrchestrator()
    orch._llm = None  # noqa: SLF001  — force keyword-heuristic path
    return orch


# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

@pytest.mark.asyncio
async def test_turn_on_living_room_executes_all_devices(session: AsyncSession) -> None:
    """'Turn on the living room' should dispatch turn_on for every device in that room."""
    orchestrator = _make_orchestrator()

    with patch(
        "app.services.device_service.emit_gateway_command",
        new_callable=AsyncMock,
        return_value=True,
    ):
        response = await orchestrator.run(
            session,
            AIChatRequest(message="Turn on the living room", conversation_id="test-turn-on-1"),
        )

    fr = response.response
    assert fr.status == "executed", (
        f"Expected 'executed' but got '{fr.status}'. "
        f"Reply: {fr.reply}  |  intent={fr.intent}  |  result={fr.result}"
    )
    assert fr.intent.action == "turn_on"
    assert fr.result is not None
    assert fr.result.executed is True
    assert len(fr.result.items) == 2
    assert all(item.status == "ok" for item in fr.result.items)


@pytest.mark.asyncio
async def test_turn_off_living_room_executes(session: AsyncSession) -> None:
    """'Turn off living room' should dispatch turn_off for every device."""
    orchestrator = _make_orchestrator()

    with patch(
        "app.services.device_service.emit_gateway_command",
        new_callable=AsyncMock,
        return_value=True,
    ):
        response = await orchestrator.run(
            session,
            AIChatRequest(message="Turn off living room", conversation_id="test-turn-off-1"),
        )

    fr = response.response
    assert fr.status == "executed", (
        f"Expected 'executed' but got '{fr.status}'. "
        f"Reply: {fr.reply}  |  intent={fr.intent}  |  result={fr.result}"
    )
    assert fr.intent.action == "turn_off"
    assert fr.result is not None
    assert fr.result.executed is True


@pytest.mark.asyncio
async def test_switch_on_bedroom_gives_clarification_when_room_missing(
    session: AsyncSession,
) -> None:
    """'Switch on the bedroom' should ask for clarification because no such room exists."""
    orchestrator = _make_orchestrator()

    with patch(
        "app.services.device_service.emit_gateway_command",
        new_callable=AsyncMock,
        return_value=True,
    ):
        response = await orchestrator.run(
            session,
            AIChatRequest(message="Switch on the bedroom", conversation_id="test-switch-on-1"),
        )

    fr = response.response
    # Room doesn't exist → safety/tool should ask for clarification
    assert fr.status == "clarification", (
        f"Expected 'clarification' but got '{fr.status}'. "
        f"Reply: {fr.reply}  |  intent={fr.intent}"
    )


@pytest.mark.asyncio
async def test_nlu_fallback_classifies_turn_on_correctly() -> None:
    """NLU keyword heuristic should detect 'turn on' intent with room hint."""
    orchestrator = _make_orchestrator()

    output = await orchestrator._nlu(  # noqa: SLF001
        {
            "request": AIChatRequest(
                message="Turn on the living room",
                conversation_id="nlu-test-1",
            ),
            "pending_plan": None,
        }
    )

    assert output["intent"].action == "turn_on"
    assert output["intent"].confidence == 1.0
    assert output["hint"].room is not None
    assert "living room" in output["hint"].room.lower()


@pytest.mark.asyncio
async def test_nlu_fallback_classifies_turn_off_correctly() -> None:
    """NLU keyword heuristic should detect 'turn off' intent."""
    orchestrator = _make_orchestrator()

    output = await orchestrator._nlu(  # noqa: SLF001
        {
            "request": AIChatRequest(
                message="Turn off the kitchen lights",
                conversation_id="nlu-test-2",
            ),
            "pending_plan": None,
        }
    )

    assert output["intent"].action == "turn_off"
    assert output["hint"].room is not None


@pytest.mark.asyncio
async def test_nlu_fallback_classifies_set_brightness() -> None:
    """NLU keyword heuristic should detect 'set brightness' with a value."""
    orchestrator = _make_orchestrator()

    output = await orchestrator._nlu(  # noqa: SLF001
        {
            "request": AIChatRequest(
                message="Set brightness to 500 in the living room",
                conversation_id="nlu-test-3",
            ),
            "pending_plan": None,
        }
    )

    assert output["intent"].action == "set_brightness"
    assert output["hint"].value == 500
    assert output["hint"].room is not None


@pytest.mark.asyncio
async def test_nlu_fallback_still_detects_conversation() -> None:
    """Messages that are NOT device commands should still route to conversation."""
    orchestrator = _make_orchestrator()

    output = await orchestrator._nlu(  # noqa: SLF001
        {
            "request": AIChatRequest(
                message="Hello, how are you?",
                conversation_id="nlu-test-4",
            ),
            "pending_plan": None,
        }
    )

    assert output["intent"].action == "conversation"
