from pydantic import BaseModel, ConfigDict


class ClientCreate(BaseModel):
    client_id: str
    server_id: str
    pwd: str
    ip: str


class ClientRead(BaseModel):
    model_config = ConfigDict(from_attributes=True)

    client_id: str
    server_id: str
    ip: str
