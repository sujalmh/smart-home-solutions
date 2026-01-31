from pydantic import BaseModel, ConfigDict


class ServerCreate(BaseModel):
    server_id: str
    pwd: str
    ip: str


class ServerRead(BaseModel):
    model_config = ConfigDict(from_attributes=True)

    server_id: str
    ip: str
