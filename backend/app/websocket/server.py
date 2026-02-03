from __future__ import annotations

import asyncio
import json
import logging

from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from jose import JWTError, jwt
from starlette.websockets import WebSocketState

from ..core.config import settings
from ..db.session import AsyncSessionLocal
from ..models.client import Client
from ..models.server import Server
from ..models.switch_module import SwitchModule

logger = logging.getLogger("websocket")


def _normalize_id(value: str) -> str:
    if value.startswith("RSW-"):
        return value
    return f"RSW-{value}"


def _strip_prefix(value: str | None) -> str | None:
    if not value:
        return None
    if value.startswith("RSW-"):
        return value[4:]
    return value


def _safe_int(value, default: int = 0) -> int:
    try:
        return int(value)
    except (TypeError, ValueError):
        return default


async def _get_client(session, client_id: str | None) -> Client | None:
    if not client_id:
        return None
    full_id = _normalize_id(client_id)
    client = await session.get(Client, full_id)
    if client is None and full_id != client_id:
        client = await session.get(Client, client_id)
    return client


async def _ensure_server(session, server_id: str | None) -> Server | None:
    if not server_id:
        return None
    full_id = _normalize_id(server_id)
    server = await session.get(Server, full_id)
    if server is None:
        pwd = full_id[4:] if full_id.startswith("RSW-") else full_id
        server = Server(server_id=full_id, pwd=pwd, ip="192.168.4.100")
        session.add(server)
        await session.commit()
    return server


async def _ensure_client(
    session, client_id: str | None, server_id: str | None
) -> Client | None:
    if not client_id:
        return None
    full_id = _normalize_id(client_id)
    client = await session.get(Client, full_id)
    if client is None:
        server_full = _normalize_id(server_id) if server_id else None
        pwd = full_id[4:] if full_id.startswith("RSW-") else full_id
        client = Client(
            client_id=full_id,
            server_id=server_full or "",
            pwd=pwd,
            ip="0.0.0.0",
        )
        session.add(client)
        await session.commit()
    return client


async def _upsert_module(
    session,
    client_id: str | None,
    comp: str | None,
    mod: int,
    stat: int,
    val: int,
) -> SwitchModule | None:
    if not client_id or not comp:
        return None
    module = await session.get(SwitchModule, {"client_id": client_id, "comp_id": comp})
    if module is None:
        module = SwitchModule(
            client_id=client_id,
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
    await session.commit()
    return module


def _build_message(event: str, data: dict) -> str:
    return json.dumps({"event": event, "data": data})


class WebSocketManager:
    def __init__(self) -> None:
        self._clients: set[WebSocket] = set()
        self._gateways: dict[str, WebSocket] = {}
        self._gateway_by_ws: dict[WebSocket, str] = {}
        self._seen: dict[str, dict[str, str]] = {}
        self._lock = asyncio.Lock()

    async def add_client(self, websocket: WebSocket) -> None:
        async with self._lock:
            self._clients.add(websocket)

    async def register_gateway(self, websocket: WebSocket, server_id: str) -> None:
        async with self._lock:
            self._clients.discard(websocket)
            existing = self._gateways.get(server_id)
            if existing and existing != websocket:
                self._gateway_by_ws.pop(existing, None)
            self._gateways[server_id] = websocket
            self._gateway_by_ws[websocket] = server_id

    async def remove(self, websocket: WebSocket) -> None:
        async with self._lock:
            self._clients.discard(websocket)
            server_id = self._gateway_by_ws.pop(websocket, None)
            if server_id and self._gateways.get(server_id) == websocket:
                self._gateways.pop(server_id, None)

    async def broadcast(self, event: str, data: dict) -> None:
        message = _build_message(event, data)
        async with self._lock:
            clients = list(self._clients)
        for websocket in clients:
            if not self._is_connected(websocket):
                await self.remove(websocket)
                continue
            try:
                await websocket.send_text(message)
            except Exception:
                await self.remove(websocket)

    async def send_to_gateway(self, server_id: str, event: str, data: dict) -> bool:
        message = _build_message(event, data)
        async with self._lock:
            websocket = self._gateways.get(server_id)
        if websocket is None:
            return False
        if not self._is_connected(websocket):
            await self.remove(websocket)
            return False
        try:
            await websocket.send_text(message)
            return True
        except Exception:
            await self.remove(websocket)
            return False

    def is_gateway_connected(self, server_id: str) -> bool:
        websocket = self._gateways.get(server_id)
        return self._is_connected(websocket) if websocket else False

    async def record_seen(self, server_id: str, client_id: str, ip: str) -> None:
        async with self._lock:
            bucket = self._seen.setdefault(server_id, {})
            bucket[client_id] = ip

    async def list_seen(self, server_id: str) -> list[dict[str, str]]:
        async with self._lock:
            bucket = dict(self._seen.get(server_id, {}))
        return [{"clientID": client_id, "ip": ip} for client_id, ip in bucket.items()]

    @staticmethod
    def _is_connected(websocket: WebSocket) -> bool:
        return websocket.client_state == WebSocketState.CONNECTED


ws_manager = WebSocketManager()


async def emit_gateway_command(
    server_id: str | None,
    dev_id: str | None,
    comp: str | None,
    mod: int,
    stat: int,
    val: int,
) -> None:
    wire_server_id = _strip_prefix(server_id)
    wire_dev_id = _strip_prefix(dev_id)
    if not wire_server_id or not wire_dev_id or not comp:
        return
    payload = {
        "serverID": wire_server_id,
        "devID": wire_dev_id,
        "comp": comp,
        "mod": mod,
        "stat": stat,
        "val": val,
    }
    await ws_manager.send_to_gateway(wire_server_id, "command", payload)


async def emit_gateway_status(
    server_id: str | None,
    dev_id: str | None,
    comp: str | None,
) -> None:
    wire_server_id = _strip_prefix(server_id)
    wire_dev_id = _strip_prefix(dev_id)
    if not wire_server_id or not wire_dev_id:
        return
    payload = {
        "serverID": wire_server_id,
        "devID": wire_dev_id,
        "comp": comp,
    }
    await ws_manager.send_to_gateway(wire_server_id, "status", payload)


async def emit_gateway_bind(server_id: str | None, client_id: str | None) -> None:
    wire_server_id = _strip_prefix(server_id)
    wire_client_id = _strip_prefix(client_id)
    if not wire_server_id or not wire_client_id:
        return
    payload = {"serverID": wire_server_id, "clientID": wire_client_id}
    await ws_manager.send_to_gateway(wire_server_id, "bind_slave", payload)


async def emit_gateway_unbind(server_id: str | None, client_id: str | None) -> None:
    wire_server_id = _strip_prefix(server_id)
    wire_client_id = _strip_prefix(client_id)
    if not wire_server_id or not wire_client_id:
        return
    payload = {"serverID": wire_server_id, "clientID": wire_client_id}
    await ws_manager.send_to_gateway(wire_server_id, "unbind_slave", payload)


def is_gateway_connected(server_id: str | None) -> bool:
    wire_server_id = _strip_prefix(server_id)
    if not wire_server_id:
        return False
    return ws_manager.is_gateway_connected(wire_server_id)


async def list_seen_slaves(server_id: str | None) -> list[dict[str, str]]:
    wire_server_id = _strip_prefix(server_id)
    if not wire_server_id:
        return []
    return await ws_manager.list_seen(wire_server_id)


async def _handle_gateway_register(websocket: WebSocket, data: dict) -> None:
    server_id = data.get("serverID")
    ip = data.get("ip")
    wire_id = _strip_prefix(server_id if isinstance(server_id, str) else None)
    if not wire_id:
        return
    await ws_manager.register_gateway(websocket, wire_id)
    async with AsyncSessionLocal() as session:
        server = await _ensure_server(session, wire_id)
        if server and isinstance(ip, str) and ip:
            server.ip = ip
            await session.commit()
    logger.info("gateway registered server=%s", wire_id)


async def _handle_register(data: dict) -> None:
    server_id = data.get("serverID")
    client_id = data.get("clientID") or data.get("devID")
    ip = data.get("ip")
    if not isinstance(server_id, str) or not isinstance(client_id, str):
        return
    async with AsyncSessionLocal() as session:
        server = await _ensure_server(session, server_id)
        client = await _ensure_client(session, client_id, server_id)
        if client and isinstance(ip, str) and ip:
            client.ip = ip
            if server:
                client.server_id = server.server_id
            await session.commit()

    wire_server_id = _strip_prefix(server_id)
    wire_client_id = _strip_prefix(client_id)
    if not isinstance(ip, str):
        ip = ""
    if wire_server_id and wire_client_id:
        await ws_manager.record_seen(wire_server_id, wire_client_id, ip)


async def _handle_slave_seen(data: dict) -> None:
    server_id = data.get("serverID")
    client_id = data.get("clientID") or data.get("devID")
    ip = data.get("ip")
    if not isinstance(server_id, str) or not isinstance(client_id, str):
        return
    if not isinstance(ip, str):
        ip = ""
    wire_server_id = _strip_prefix(server_id)
    wire_client_id = _strip_prefix(client_id)
    if not wire_server_id or not wire_client_id:
        return
    await ws_manager.record_seen(wire_server_id, wire_client_id, ip)


async def _handle_status_event(event: str, data: dict) -> None:
    dev_id = data.get("devID")
    comp = data.get("comp")
    mod = _safe_int(data.get("mod"))
    stat = _safe_int(data.get("stat"))
    val = _safe_int(data.get("val"))
    if not isinstance(dev_id, str) or not isinstance(comp, str):
        return

    async with AsyncSessionLocal() as session:
        client = await _get_client(session, dev_id)
        if client:
            await _upsert_module(session, client.client_id, comp, mod, stat, val)

    await ws_manager.broadcast(
        event,
        {"devID": dev_id, "comp": comp, "mod": mod, "stat": stat, "val": val},
    )


async def _handle_command(data: dict) -> None:
    server_id = data.get("serverID")
    dev_id = data.get("devID")
    comp = data.get("comp")
    mod = _safe_int(data.get("mod"))
    stat = _safe_int(data.get("stat"))
    val = _safe_int(data.get("val"))
    await emit_gateway_command(server_id, dev_id, comp, mod, stat, val)


async def _handle_status_request(data: dict) -> None:
    server_id = data.get("serverID")
    dev_id = data.get("devID")
    comp = data.get("comp")
    await emit_gateway_status(server_id, dev_id, comp)


async def _handle_message(websocket: WebSocket, message: str) -> None:
    try:
        payload = json.loads(message)
    except json.JSONDecodeError:
        logger.warning("websocket invalid json")
        return
    if not isinstance(payload, dict):
        return
    event = payload.get("event")
    data = payload.get("data")
    if not isinstance(event, str) or not isinstance(data, dict):
        return

    if event == "gateway_register":
        await _handle_gateway_register(websocket, data)
    elif event == "register":
        await _handle_register(data)
    elif event in ("staresult", "response", "register_status"):
        mapped_event = "staresult" if event == "register_status" else event
        await _handle_status_event(mapped_event, data)
    elif event == "command":
        await _handle_command(data)
    elif event == "status":
        await _handle_status_request(data)
    elif event == "bind_slave":
        await emit_gateway_bind(data.get("serverID"), data.get("clientID"))
    elif event == "unbind_slave":
        await emit_gateway_unbind(data.get("serverID"), data.get("clientID"))
    elif event == "slave_seen":
        await _handle_slave_seen(data)


def register_websocket_routes(app: FastAPI) -> None:
    @app.websocket("/ws")
    async def websocket_endpoint(websocket: WebSocket):
        token = websocket.query_params.get("token")
        if token:
            try:
                jwt.decode(
                    token, settings.jwt_secret_key, algorithms=[settings.jwt_algorithm]
                )
            except JWTError:
                await websocket.accept()
                await websocket.close(code=1008)
                return

        await websocket.accept()
        await ws_manager.add_client(websocket)
        try:
            while True:
                message = await websocket.receive_text()
                await _handle_message(websocket, message)
        except WebSocketDisconnect:
            pass
        finally:
            await ws_manager.remove(websocket)
