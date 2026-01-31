import logging

from jose import JWTError, jwt
import socketio
from sqlalchemy import select

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


def _normalize_id(value: str | None) -> str | None:
    if not value:
        return None
    if value.startswith("RSW-"):
        return value
    return f"RSW-{value}"


def _safe_int(value, default: int = 0) -> int:
    try:
        return int(value)
    except (TypeError, ValueError):
        return default


async def _get_server(session, server_id: str | None) -> Server | None:
    if not server_id:
        return None
    full_id = _normalize_id(server_id)
    server = await session.get(Server, full_id)
    if server is None and full_id != server_id:
        server = await session.get(Server, server_id)
    return server


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


async def _ensure_client(session, client_id: str | None, server_id: str | None) -> Client | None:
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

        if dev_id:
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

    async with AsyncSessionLocal() as session:
        user = await session.get(User, email)
        if user is None:
            user = User(
                email_id=email,
                pwd=hash_password(pwd),
                device_id=_normalize_id(dev_id) or "",
            )
            session.add(user)
        else:
            user.device_id = _normalize_id(dev_id) or user.device_id
        await session.commit()

    return "success"


@sio.event
async def upd_client(sid, data):
    email = data.get("name") if isinstance(data, dict) else None
    pwd = data.get("pword") if isinstance(data, dict) else None
    dev_id = data.get("devID") if isinstance(data, dict) else None
    if not email:
        return "missing"

    async with AsyncSessionLocal() as session:
        user = await session.get(User, email)
        if user is None and pwd:
            user = User(
                email_id=email,
                pwd=hash_password(pwd),
                device_id=_normalize_id(dev_id) or "",
            )
            session.add(user)
        elif user is not None and dev_id:
            user.device_id = _normalize_id(dev_id) or user.device_id
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

    async with AsyncSessionLocal() as session:
        await _ensure_server(session, server_id)
        client = await _ensure_client(session, dev_id, server_id)
        if client:
            await _upsert_module(session, client.client_id, comp, mod, stat, val)

    await sio.emit(
        "response",
        {
            "devID": dev_id,
            "comp": comp,
            "mod": mod,
            "stat": stat,
            "val": val,
        },
    )

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

    async with AsyncSessionLocal() as session:
        client = await _get_client(session, dev_id)
        if client is None:
            return {"serverID": server_id, "devID": dev_id, "comp": comp}

        stmt = select(SwitchModule).where(SwitchModule.client_id == client.client_id)
        if comp:
            stmt = stmt.where(SwitchModule.comp_id == comp)
        result = await session.execute(stmt)
        modules = list(result.scalars().all())

    for module in modules:
        await sio.emit(
            "staresult",
            {
                "devID": module.client_id[4:] if module.client_id.startswith("RSW-") else module.client_id,
                "comp": module.comp_id,
                "mod": module.mode,
                "stat": module.status,
                "val": module.value,
            },
        )

    return {"serverID": server_id, "devID": dev_id, "comp": comp}


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
