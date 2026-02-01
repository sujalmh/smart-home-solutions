import logging

from jose import JWTError, jwt
import socketio

from ..core.config import settings
from ..core.security import hash_password, verify_password
from ..db.session import AsyncSessionLocal
from ..models.client import Client
from ..models.server import Server
from ..models.switch_module import SwitchModule
from ..models.user import User


logger = logging.getLogger("socketio")


sio = socketio.AsyncServer(
    async_mode="asgi",
    cors_allowed_origins=settings.cors_origin_list() or "*",
)

GATEWAY_ROOM_PREFIX = "gateway:"


def create_socket_app(other_asgi_app):
    return socketio.ASGIApp(
        sio,
        other_asgi_app=other_asgi_app,
        socketio_path=settings.socketio_path,
    )


@sio.event
async def connect(sid, environ, auth):
    user = None
    token = None
    if isinstance(auth, dict):
        token = auth.get("token") or auth.get("access_token")
    elif isinstance(auth, str):
        token = auth

    if token:
        try:
            payload = jwt.decode(
                token, settings.jwt_secret_key, algorithms=[settings.jwt_algorithm]
            )
            user = payload.get("sub")
        except JWTError as exc:
            logger.warning("socket auth failed sid=%s error=%s", sid, exc)
            raise ConnectionRefusedError("unauthorized")

    await sio.save_session(sid, {"user": user})
    logger.info("socket connected sid=%s user=%s", sid, user)


@sio.event
async def disconnect(sid):
    logger.info("socket disconnected sid=%s", sid)


@sio.event
async def gateway_register(sid, data):
    payload = data if isinstance(data, dict) else {}
    server_id = payload.get("serverID")
    ip = payload.get("ip")
    wire_id = _strip_prefix(server_id)
    if not wire_id:
        return "missing"

    await sio.enter_room(sid, _gateway_room(wire_id))

    async with AsyncSessionLocal() as session:
        server = await _ensure_server(session, wire_id)
        if server and ip:
            server.ip = ip
            await session.commit()

    session_data = await sio.get_session(sid)
    session_data["gateway_server_id"] = wire_id
    await sio.save_session(sid, session_data)
    logger.info("gateway registered sid=%s server=%s", sid, wire_id)
    return "ok"


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


def _gateway_room(server_id: str | None) -> str | None:
    wire_id = _strip_prefix(server_id)
    if not wire_id:
        return None
    return f"{GATEWAY_ROOM_PREFIX}{wire_id}"


async def _emit_gateway(event: str, payload: dict, server_id: str | None) -> None:
    room = _gateway_room(server_id)
    if room is None:
        return
    await sio.emit(event, payload, room=room)


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
    await _emit_gateway("command", payload, wire_server_id)


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
    await _emit_gateway("status", payload, wire_server_id)


async def _get_server(session, server_id: str | None) -> Server | None:
    if not server_id:
        return None
    assert server_id is not None
    full_id = _normalize_id(server_id)
    server = await session.get(Server, full_id)
    if server is None and full_id != server_id:
        server = await session.get(Server, server_id)
    return server


async def _get_client(session, client_id: str | None) -> Client | None:
    if not client_id:
        return None
    assert client_id is not None
    full_id = _normalize_id(client_id)
    client = await session.get(Client, full_id)
    if client is None and full_id != client_id:
        client = await session.get(Client, client_id)
    return client


async def _ensure_server(session, server_id: str | None) -> Server | None:
    if not server_id:
        return None
    assert server_id is not None
    full_id = _normalize_id(server_id)
    server = await session.get(Server, full_id)
    if server is None:
        pwd = full_id[4:] if full_id.startswith("RSW-") else full_id
        server = Server(server_id=full_id, pwd=pwd, ip="192.168.4.100")
        session.add(server)
        await session.commit()
    return server


async def _ensure_client(session, client_id: str | None, server_id: str | None) -> Client | None:
    if not client_id:
        return None
    assert client_id is not None
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


@sio.event
async def login(sid, data):
    email = data.get("name") if isinstance(data, dict) else None
    pwd = data.get("pword") if isinstance(data, dict) else None
    dev_id = data.get("devID") if isinstance(data, dict) else None
    if not email or not pwd:
        return "missing"

    async with AsyncSessionLocal() as session:
        user = await session.get(User, email)
        if not user or not verify_password(pwd, user.pwd):
            return "invalid"

        if isinstance(dev_id, str) and dev_id:
            user.device_id = _normalize_id(dev_id) or user.device_id
            await session.commit()

    return "success"


@sio.event
async def new_user(sid, data):
    email = data.get("name") if isinstance(data, dict) else None
    pwd = data.get("pword") if isinstance(data, dict) else None
    dev_id = data.get("devID") if isinstance(data, dict) else None
    if not email or not pwd:
        return "missing"

    normalized_dev_id = _normalize_id(dev_id) if isinstance(dev_id, str) and dev_id else None

    async with AsyncSessionLocal() as session:
        user = await session.get(User, email)
        if user is None:
            user = User(
                email_id=email,
                pwd=hash_password(pwd),
                device_id=normalized_dev_id or "",
            )
            session.add(user)
        else:
            if normalized_dev_id:
                user.device_id = normalized_dev_id or user.device_id
        await session.commit()

    return "success"


@sio.event
async def upd_client(sid, data):
    email = data.get("name") if isinstance(data, dict) else None
    pwd = data.get("pword") if isinstance(data, dict) else None
    dev_id = data.get("devID") if isinstance(data, dict) else None
    if not email:
        return "missing"

    normalized_dev_id = _normalize_id(dev_id) if isinstance(dev_id, str) and dev_id else None

    async with AsyncSessionLocal() as session:
        user = await session.get(User, email)
        if user is None and pwd:
            user = User(
                email_id=email,
                pwd=hash_password(pwd),
                device_id=normalized_dev_id or "",
            )
            session.add(user)
        elif user is not None and normalized_dev_id:
            user.device_id = normalized_dev_id or user.device_id
        if user is not None:
            await session.commit()

    return "success"


@sio.event
async def command(sid, data):
    payload = data if isinstance(data, dict) else {}
    server_id = payload.get("serverID")
    dev_id = payload.get("devID")
    comp = payload.get("comp")
    mod = _safe_int(payload.get("mod"))
    stat = _safe_int(payload.get("stat"))
    val = _safe_int(payload.get("val"))

    await emit_gateway_command(server_id, dev_id, comp, mod, stat, val)

    async with AsyncSessionLocal() as session:
        await _ensure_server(session, server_id)
        await _ensure_client(session, dev_id, server_id)

    return {
        "serverID": server_id,
        "devID": dev_id,
        "comp": comp,
        "mod": mod,
        "stat": stat,
        "val": val,
    }


@sio.event
async def status(sid, data):
    payload = data if isinstance(data, dict) else {}
    server_id = payload.get("serverID")
    dev_id = payload.get("devID")
    comp = payload.get("comp")

    await emit_gateway_status(server_id, dev_id, comp)

    return {"serverID": server_id, "devID": dev_id, "comp": comp}


@sio.event
async def register(sid, data):
    payload = data if isinstance(data, dict) else {}
    server_id = payload.get("serverID")
    client_id = payload.get("clientID") or payload.get("devID")
    ip = payload.get("ip")
    if not server_id or not client_id:
        return "missing"

    async with AsyncSessionLocal() as session:
        server = await _ensure_server(session, server_id)
        client = await _ensure_client(session, client_id, server_id)
        if client and ip:
            client.ip = ip
            if server:
                client.server_id = server.server_id
            await session.commit()

    return "ok"


@sio.event
async def staresult(sid, data):
    payload = data if isinstance(data, dict) else {}
    dev_id = payload.get("devID")
    comp = payload.get("comp")
    mod = _safe_int(payload.get("mod"))
    stat = _safe_int(payload.get("stat"))
    val = _safe_int(payload.get("val"))

    async with AsyncSessionLocal() as session:
        client = await _get_client(session, dev_id)
        if client:
            await _upsert_module(session, client.client_id, comp, mod, stat, val)

    await sio.emit(
        "staresult",
        {"devID": dev_id, "comp": comp, "mod": mod, "stat": stat, "val": val},
    )


@sio.event
async def response(sid, data):
    payload = data if isinstance(data, dict) else {}
    dev_id = payload.get("devID")
    comp = payload.get("comp")
    mod = _safe_int(payload.get("mod"))
    stat = _safe_int(payload.get("stat"))
    val = _safe_int(payload.get("val"))

    async with AsyncSessionLocal() as session:
        client = await _get_client(session, dev_id)
        if client:
            await _upsert_module(session, client.client_id, comp, mod, stat, val)

    await sio.emit(
        "response",
        {"devID": dev_id, "comp": comp, "mod": mod, "stat": stat, "val": val},
    )
