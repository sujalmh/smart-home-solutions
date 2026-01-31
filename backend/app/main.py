from contextlib import asynccontextmanager

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

from .api.router import api_router
from .core.config import settings
from .core.logging import configure_logging
from .socketio.server import create_socket_app
from .db.init_db import init_db
from .db.session import engine


configure_logging()


@asynccontextmanager
async def lifespan(app: FastAPI):
    await init_db(engine)
    yield


def build_fastapi_app() -> FastAPI:
    app = FastAPI(title=settings.app_name, lifespan=lifespan)
    app.include_router(api_router, prefix=settings.api_prefix)

    origins = settings.cors_origin_list()
    if origins:
        app.add_middleware(
            CORSMiddleware,
            allow_origins=origins,
            allow_credentials=True,
            allow_methods=["*"],
            allow_headers=["*"],
        )

    return app


fastapi_app = build_fastapi_app()
app = create_socket_app(fastapi_app)
