from __future__ import annotations

from pathlib import Path

import pytest
import pytest_asyncio
from sqlalchemy.ext.asyncio import AsyncSession, async_sessionmaker, create_async_engine

from app.ai.orchestrator import AIOrchestrator
from app.ai.schemas import AIChatRequest, ExecutionPlan, RoomRef, ToolInput
from app.ai.tools import execute_status_query_tool
from app.db.base import Base
from app.models.client import Client
from app.models.device import Device
from app.models.home import Home
from app.models.room import Room
from app.models.server import Server
from app.models.switch_module import SwitchModule


@pytest_asyncio.fixture
async def session(tmp_path: Path) -> AsyncSession:
    db_path = tmp_path / "ai_status_queries.db"
    engine = create_async_engine(f"sqlite+aiosqlite:///{db_path}")
    session_maker = async_sessionmaker(engine, expire_on_commit=False)

    async with engine.begin() as conn:
        await conn.run_sync(Base.metadata.create_all)

    async with session_maker() as db_session:
        db_session.add(Home(home_id="home-1", name="Home 1"))
        db_session.add(Room(room_id="room-1", home_id="home-1", name="Living Room"))

        db_session.add(Server(server_id="RSW-1001", pwd="1001", ip="192.168.0.10"))
        db_session.add(Server(server_id="RSW-1002", pwd="1002", ip="192.168.0.11"))

        db_session.add(Client(client_id="RSW-2001", server_id="RSW-1001", pwd="2001", ip=""))
        db_session.add(Client(client_id="RSW-2002", server_id="RSW-1002", pwd="2002", ip=""))

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
                server_id=None,
                client_id=None,
                name="Wall Light",
                device_type="light",
                is_active=True,
            )
        )

        db_session.add(
            SwitchModule(
                client_id="RSW-2001",
                comp_id="Comp0",
                mode=1,
                status=1,
                value=1000,
            )
        )
        db_session.add(
            SwitchModule(
                client_id="RSW-2001",
                comp_id="Comp1",
                mode=1,
                status=0,
                value=0,
            )
        )
        await db_session.commit()
        yield db_session

    await engine.dispose()


@pytest.mark.asyncio
async def test_status_query_returns_global_counts(session: AsyncSession) -> None:
    payload = ToolInput(
        plan=ExecutionPlan(
            action="status_check",
            room=None,
            devices=[],
            value=None,
            comp="Comp0",
            source="ai",
        )
    )

    result = await execute_status_query_tool(
        session,
        payload,
        "How many gateways and switches are connected?",
    )

    assert result.status == "executed"
    assert result.executed is True
    assert result.summary is not None
    assert "Gateways: 2 total" in result.summary
    assert "Switches: 2 total" in result.summary


@pytest.mark.asyncio
async def test_status_query_returns_room_scoped_counts(session: AsyncSession) -> None:
    payload = ToolInput(
        plan=ExecutionPlan(
            action="status_check",
            room=RoomRef(id="room-1", name="Living Room"),
            devices=[],
            value=None,
            comp="Comp0",
            source="ai",
        )
    )

    result = await execute_status_query_tool(
        session,
        payload,
        "How many devices and switches are there in living room?",
    )

    assert result.summary is not None
    assert "Devices in Living Room: 2 active, 1 connected" in result.summary
    assert "Switches in Living Room: 2 total, 1 on" in result.summary


@pytest.mark.asyncio
async def test_nlu_fallback_detects_inventory_queries() -> None:
    orchestrator = AIOrchestrator()
    orchestrator._llm = None  # noqa: SLF001

    output = await orchestrator._nlu(  # noqa: SLF001
        {
            "request": AIChatRequest(
                message="How many devices are connected right now?",
                conversation_id="fallback-1",
            ),
            "pending_plan": None,
        }
    )

    assert output["intent"].action == "status_check"
    assert output["intent"].confidence == 1.0


@pytest.mark.asyncio
async def test_nlu_fallback_detects_non_count_inventory_question() -> None:
    orchestrator = AIOrchestrator()
    orchestrator._llm = None  # noqa: SLF001

    output = await orchestrator._nlu(  # noqa: SLF001
        {
            "request": AIChatRequest(
                message="What devices are connected?",
                conversation_id="fallback-2",
            ),
            "pending_plan": None,
        }
    )

    assert output["intent"].action == "status_check"
    assert output["intent"].confidence == 1.0


@pytest.mark.asyncio
async def test_status_query_listing_includes_connected_device_names(
    session: AsyncSession,
) -> None:
    payload = ToolInput(
        plan=ExecutionPlan(
            action="status_check",
            room=RoomRef(id="room-1", name="Living Room"),
            devices=[],
            value=None,
            comp="Comp0",
            source="ai",
        )
    )

    result = await execute_status_query_tool(
        session,
        payload,
        "What devices are connected in living room?",
    )

    assert result.summary is not None
    assert "Connected devices:" in result.summary
    assert "Ceiling Light" in result.summary
