from fastapi import APIRouter
from .health import router as health_router
from .auth import router as auth_router
from .servers import router as servers_router
from .clients import router as clients_router
from .devices import router as devices_router


api_router = APIRouter()
api_router.include_router(health_router)
api_router.include_router(auth_router)
api_router.include_router(servers_router)
api_router.include_router(clients_router)
api_router.include_router(devices_router)