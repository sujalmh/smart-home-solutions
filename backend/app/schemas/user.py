from pydantic import BaseModel, ConfigDict


class UserCreate(BaseModel):
    email_id: str
    pwd: str
    device_id: str


class UserRead(BaseModel):
    model_config = ConfigDict(from_attributes=True)

    email_id: str
    device_id: str
