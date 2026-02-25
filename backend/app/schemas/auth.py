from pydantic import BaseModel


class RegisterRequest(BaseModel):
    email_id: str
    pwd: str
    device_id: str | None = None


class LoginRequest(BaseModel):
    email_id: str
    pwd: str
    device_id: str | None = None


class Token(BaseModel):
    access_token: str
    token_type: str = "bearer"
