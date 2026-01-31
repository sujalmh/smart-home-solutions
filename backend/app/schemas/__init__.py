from .user import UserCreate, UserRead
from .auth import LoginRequest, RegisterRequest, Token
from .device import (
    DeviceClientConfigRequest,
    DeviceCommandRequest,
    DeviceCommandResponse,
    DeviceConfigResponse,
    DeviceRegisterRequest,
    DeviceRemoteConfigRequest,
    DeviceServerConfigRequest,
    DeviceStatusRequest,
)
from .server import ServerCreate, ServerRead
from .client import ClientCreate, ClientRead
from .switch_module import SwitchModuleCreate, SwitchModuleRead, SwitchModuleUpdate

__all__ = [
    "UserCreate",
    "UserRead",
    "LoginRequest",
    "RegisterRequest",
    "Token",
    "DeviceClientConfigRequest",
    "DeviceCommandRequest",
    "DeviceCommandResponse",
    "DeviceConfigResponse",
    "DeviceRegisterRequest",
    "DeviceRemoteConfigRequest",
    "DeviceServerConfigRequest",
    "DeviceStatusRequest",
    "ServerCreate",
    "ServerRead",
    "ClientCreate",
    "ClientRead",
    "SwitchModuleCreate",
    "SwitchModuleRead",
    "SwitchModuleUpdate",
]
