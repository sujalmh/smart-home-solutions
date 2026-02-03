from pydantic import BaseModel, ConfigDict, Field


class DeviceCommandRequest(BaseModel):
    model_config = ConfigDict(populate_by_name=True, serialize_by_alias=True)

    server_id: str = Field(alias="serverID")
    dev_id: str = Field(alias="devID")
    comp: str
    mod: int = Field(ge=0, le=1)
    stat: int = Field(ge=0, le=1)
    val: int = Field(ge=0, le=1000)


class DeviceCommandResponse(DeviceCommandRequest):
    pass


class DeviceStatusRequest(BaseModel):
    model_config = ConfigDict(populate_by_name=True, serialize_by_alias=True)

    server_id: str = Field(alias="serverID")
    dev_id: str = Field(alias="devID")
    comp: str | None = None


class DeviceServerConfigRequest(BaseModel):
    server_id: str
    pwd: str | None = None
    ip: str | None = None


class DeviceRemoteConfigRequest(BaseModel):
    ssid: str
    pwd: str


class DeviceClientConfigRequest(BaseModel):
    client_id: str
    pwd: str
    ip: str


class DeviceRegisterRequest(BaseModel):
    server_id: str


class DeviceBindRequest(BaseModel):
    client_id: str


class DeviceConfigResponse(BaseModel):
    status: str
