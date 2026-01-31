from pydantic import BaseModel, ConfigDict, Field


class SwitchModuleCreate(BaseModel):
    client_id: str
    comp_id: str
    mode: int = Field(ge=0, le=1)
    status: int = Field(ge=0, le=1)
    value: int = Field(ge=0, le=1000)


class SwitchModuleUpdate(BaseModel):
    mode: int = Field(ge=0, le=1)
    status: int = Field(ge=0, le=1)
    value: int = Field(ge=0, le=1000)


class SwitchModuleRead(BaseModel):
    model_config = ConfigDict(from_attributes=True)

    client_id: str
    comp_id: str
    mode: int
    status: int
    value: int
