from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy.ext.asyncio import AsyncSession

from ..core.security import create_access_token, hash_password, verify_password
from ..db.session import get_async_session
from ..models.user import User
from ..schemas.auth import LoginRequest, RegisterRequest, Token
from ..schemas.user import UserRead


router = APIRouter(prefix="/auth", tags=["auth"])


@router.post("/register", response_model=UserRead)
async def register(
    payload: RegisterRequest,
    session: AsyncSession = Depends(get_async_session),
) -> UserRead:
    existing = await session.get(User, payload.email_id)
    if existing:
        return existing

    user = User(
        email_id=payload.email_id,
        pwd=hash_password(payload.pwd),
        device_id=payload.device_id,
    )
    session.add(user)
    await session.commit()
    await session.refresh(user)
    return user


@router.post("/login", response_model=Token)
async def login(
    payload: LoginRequest,
    session: AsyncSession = Depends(get_async_session),
) -> Token:
    user = await session.get(User, payload.email_id)
    if not user or not verify_password(payload.pwd, user.pwd):
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="invalid_credentials",
        )

    access_token = create_access_token(payload.email_id)
    return Token(access_token=access_token)
