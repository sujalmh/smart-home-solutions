from pydantic import BaseModel, ConfigDict, Field


class HomeRead(BaseModel):
    model_config = ConfigDict(from_attributes=True)

    home_id: str
    name: str


class RoomRead(BaseModel):
    model_config = ConfigDict(from_attributes=True)

    room_id: str
    home_id: str
    name: str


class RoomDeviceRead(BaseModel):
    model_config = ConfigDict(from_attributes=True)

    device_id: str
    room_id: str
    server_id: str | None
    client_id: str | None
    name: str
    device_type: str
    is_active: bool


class RoomDeviceCommandRequest(BaseModel):
    comp: str = Field(min_length=1, max_length=64)
    mod: int = Field(ge=0, le=1)
    stat: int = Field(ge=0, le=1)
    val: int = Field(ge=0, le=1000)
    source: str = Field(default="api", min_length=1, max_length=32)


class RoomDeviceCommandResponse(BaseModel):
    room_id: str
    device_id: str
    server_id: str
    dev_id: str
    comp: str
    mod: int
    stat: int
    val: int
