from __future__ import annotations

from datetime import datetime
from typing import Literal

from pydantic import BaseModel, ConfigDict, Field


ActionType = Literal[
    "turn_on",
    "turn_off",
    "set_brightness",
    "status_check",
    "schedule_action",
    "unknown",
]

ExecutionStatus = Literal["executed", "clarification", "confirmation", "denied", "no_action"]
SafetyDecisionType = Literal["allow", "clarify", "confirm", "deny"]


class RoomRef(BaseModel):
    id: str
    name: str


class DeviceRef(BaseModel):
    id: str
    name: str
    type: str


class Intent(BaseModel):
    action: ActionType
    confidence: float = Field(ge=0.0, le=1.0)


class NLUHint(BaseModel):
    room: str | None = None
    device: str | None = None
    comp: str | None = Field(default=None, min_length=1, max_length=64)
    value: int | None = Field(default=None, ge=0, le=1000)
    time: str | None = None


class NLUResult(BaseModel):
    intent: Intent
    hint: NLUHint = Field(default_factory=NLUHint)


class ResolvedEntities(BaseModel):
    room: RoomRef | None = None
    devices: list[DeviceRef] = Field(default_factory=list)
    action: ActionType
    value: int | None = Field(default=None, ge=0, le=1000)
    comp: str | None = Field(default=None, min_length=1, max_length=64)


class ExecutionPlan(BaseModel):
    model_config = ConfigDict(extra="forbid")

    action: ActionType
    room: RoomRef | None = None
    devices: list[DeviceRef] = Field(default_factory=list)
    value: int | None = Field(default=None, ge=0, le=1000)
    comp: str = Field(default="Comp0", min_length=1, max_length=64)
    source: Literal["ai"] = "ai"


class ToolInput(BaseModel):
    model_config = ConfigDict(extra="forbid")

    plan: ExecutionPlan


class ToolExecutionItem(BaseModel):
    device: DeviceRef
    status: Literal["ok", "error"]
    detail: str


class ToolResult(BaseModel):
    status: ExecutionStatus
    executed: bool
    reason_codes: list[str] = Field(default_factory=list)
    items: list[ToolExecutionItem] = Field(default_factory=list)
    summary: str | None = None


class SafetyDecision(BaseModel):
    decision: SafetyDecisionType
    reason_codes: list[str] = Field(default_factory=list)


class FinalResponse(BaseModel):
    status: ExecutionStatus
    reply: str
    requires_clarification: bool = False
    requires_confirmation: bool = False
    intent: Intent
    entities: ResolvedEntities
    plan: ExecutionPlan | None = None
    result: ToolResult | None = None


class PersistedState(BaseModel):
    intent: Intent | None = None
    entities: ResolvedEntities | None = None
    plan: ExecutionPlan | None = None
    decision: SafetyDecision | None = None
    result: ToolResult | None = None


class AIChatRequest(BaseModel):
    message: str = Field(min_length=1, max_length=1000)
    conversation_id: str = Field(default="default", min_length=1, max_length=64)
    confirm: bool | None = None


class AIChatResponse(BaseModel):
    conversation_id: str
    response: FinalResponse
    expires_at: datetime | None = None
