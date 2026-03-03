"""Tests for cross-master device binding isolation.

Verifies that:
1. gateway_seen excludes slaves bound to ANY server (not just the requester)
2. gateway_bind returns 409 if the slave is already bound elsewhere
3. _handle_register does not overwrite server_id for cross-server slaves
"""

from __future__ import annotations

from pathlib import Path
from unittest.mock import AsyncMock, patch

import pytest
import pytest_asyncio
from httpx import ASGITransport, AsyncClient
from sqlalchemy.ext.asyncio import AsyncSession, async_sessionmaker, create_async_engine

from app.core.security import create_access_token
from app.db.base import Base
from app.models.client import Client
from app.models.server import Server
from app.models.user import User


@pytest_asyncio.fixture
async def engine(tmp_path: Path):
    db_path = tmp_path / "cross_master.db"
    eng = create_async_engine(f"sqlite+aiosqlite:///{db_path}")
    async with eng.begin() as conn:
        await conn.run_sync(Base.metadata.create_all)
    yield eng
    await eng.dispose()


@pytest_asyncio.fixture
async def session_factory(engine):
    return async_sessionmaker(engine, expire_on_commit=False)


@pytest_asyncio.fixture
async def seed(session_factory):
    """Seed two servers, one user, and one client bound to server A."""
    async with session_factory() as s:
        s.add(Server(server_id="RSW-0001", pwd="0001", ip="10.0.0.1"))
        s.add(Server(server_id="RSW-0002", pwd="0002", ip="10.0.0.2"))
        s.add(
            Client(
                client_id="RSW-9001",
                server_id="RSW-0001",
                pwd="9001",
                ip="10.0.0.101",
            )
        )
        s.add(
            User(
                email_id="test@local.dev",
                pwd="hashed",
            )
        )
        await s.commit()


@pytest_asyncio.fixture
async def client(engine, session_factory, seed):
    """Authenticated httpx AsyncClient against the FastAPI app."""
    from app.main import build_fastapi_app
    from app.db import session as session_mod

    app = build_fastapi_app()

    # Override DB session to use our test database
    async def _get_test_session():
        async with session_factory() as s:
            yield s

    app.dependency_overrides[session_mod.get_async_session] = _get_test_session

    token = create_access_token("test@local.dev")
    transport = ASGITransport(app=app)
    async with AsyncClient(transport=transport, base_url="http://test") as ac:
        ac.headers["Authorization"] = f"Bearer {token}"
        yield ac


# ── 1. gateway_seen must exclude slaves bound to OTHER servers ────────────


@pytest.mark.asyncio
async def test_gateway_seen_excludes_cross_server_bound(client: AsyncClient):
    """Server B's /gateway/seen must NOT list slave 9001 (bound to server A)."""
    fake_seen = [
        {"clientID": "9001", "ip": "10.0.0.101"},
        {"clientID": "9999", "ip": "10.0.0.199"},
    ]
    with patch(
        "app.api.devices.list_seen_slaves",
        new_callable=AsyncMock,
        return_value=fake_seen,
    ):
        resp = await client.get("/api/devices/RSW-0002/gateway/seen")

    assert resp.status_code == 200
    ids = [e["clientID"] for e in resp.json()]
    # 9001 is bound to server A → must be hidden from server B
    assert "9001" not in ids
    # 9999 is not bound anywhere → must remain visible
    assert "9999" in ids


@pytest.mark.asyncio
async def test_gateway_seen_excludes_same_server_bound(client: AsyncClient):
    """Server A's /gateway/seen must also exclude its own bound slave 9001."""
    fake_seen = [
        {"clientID": "9001", "ip": "10.0.0.101"},
        {"clientID": "9999", "ip": "10.0.0.199"},
    ]
    with patch(
        "app.api.devices.list_seen_slaves",
        new_callable=AsyncMock,
        return_value=fake_seen,
    ):
        resp = await client.get("/api/devices/RSW-0001/gateway/seen")

    assert resp.status_code == 200
    ids = [e["clientID"] for e in resp.json()]
    assert "9001" not in ids
    assert "9999" in ids


# ── 2. gateway_bind must reject already-bound slave with 409 ─────────────


@pytest.mark.asyncio
async def test_gateway_bind_rejects_cross_server(client: AsyncClient):
    """Binding slave 9001 (bound to A) from server B must return 409."""
    with patch(
        "app.api.devices.emit_gateway_bind",
        new_callable=AsyncMock,
    ) as mock_emit:
        resp = await client.post(
            "/api/devices/RSW-0002/gateway/bind",
            json={"client_id": "9001"},
        )

    assert resp.status_code == 409
    assert "already bound" in resp.json()["detail"].lower()
    mock_emit.assert_not_called()


@pytest.mark.asyncio
async def test_gateway_bind_allows_same_server(client: AsyncClient):
    """Re-binding slave 9001 from server A (its owner) must succeed."""
    with patch(
        "app.api.devices.emit_gateway_bind",
        new_callable=AsyncMock,
    ):
        resp = await client.post(
            "/api/devices/RSW-0001/gateway/bind",
            json={"client_id": "9001"},
        )

    assert resp.status_code == 200


@pytest.mark.asyncio
async def test_gateway_bind_allows_new_slave(client: AsyncClient):
    """Binding a completely new slave (not in DB) must succeed."""
    with patch(
        "app.api.devices.emit_gateway_bind",
        new_callable=AsyncMock,
    ):
        resp = await client.post(
            "/api/devices/RSW-0002/gateway/bind",
            json={"client_id": "9999"},
        )

    assert resp.status_code == 200


# ── 3. _handle_register must not overwrite cross-server binding ──────────


@pytest_asyncio.fixture
async def patched_register(session_factory):
    """Patch AsyncSessionLocal and ws_manager so _handle_register uses the test DB."""
    from app.websocket import server as ws_server

    mock_ws = AsyncMock()
    mock_ws.record_seen = AsyncMock()

    with patch.object(ws_server, "AsyncSessionLocal", session_factory), \
         patch.object(ws_server, "ws_manager", mock_ws):
        yield ws_server._handle_register


@pytest.mark.asyncio
async def test_handle_register_preserves_binding(session_factory, seed, patched_register):
    """When server B emits register for slave 9001 (bound to A), server_id stays A."""
    await patched_register(
        {"serverID": "0002", "clientID": "9001", "ip": "10.0.0.101"}
    )

    async with session_factory() as s:
        c = await s.get(Client, "RSW-9001")
        assert c is not None
        # server_id must still be server A, not overwritten to B
        assert c.server_id == "RSW-0001"


@pytest.mark.asyncio
async def test_handle_register_allows_own_server(session_factory, seed, patched_register):
    """When server A emits register for its own slave 9001, IP updates normally."""
    await patched_register(
        {"serverID": "0001", "clientID": "9001", "ip": "10.0.0.222"}
    )

    async with session_factory() as s:
        c = await s.get(Client, "RSW-9001")
        assert c is not None
        assert c.server_id == "RSW-0001"
        assert c.ip == "10.0.0.222"
