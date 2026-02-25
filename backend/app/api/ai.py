from fastapi import APIRouter, Depends
from sqlalchemy.ext.asyncio import AsyncSession

from ..ai.orchestrator import orchestrator
from ..ai.schemas import AIChatRequest, AIChatResponse
from ..db.session import get_async_session


router = APIRouter(prefix="/ai", tags=["ai"])


@router.post("/chat", response_model=AIChatResponse)
async def ai_chat(
    payload: AIChatRequest,
    session: AsyncSession = Depends(get_async_session),
) -> AIChatResponse:
    return await orchestrator.run(session, payload)
