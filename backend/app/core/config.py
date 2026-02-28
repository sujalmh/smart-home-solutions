from functools import lru_cache
from pathlib import Path

from pydantic_settings import BaseSettings, SettingsConfigDict


_ENV_FILE = Path(__file__).resolve().parents[2] / ".env"


class Settings(BaseSettings):
    model_config = SettingsConfigDict(
        env_file=str(_ENV_FILE),
        env_file_encoding="utf-8",
        extra="ignore",
    )

    app_name: str = "room-automation-backend"
    api_prefix: str = "/api"
    cors_origins: str = ""
    log_level: str = "INFO"
    database_url: str = "sqlite+aiosqlite:///./room_automation.db"
    jwt_secret_key: str = "change-me"
    jwt_algorithm: str = "HS256"
    jwt_access_token_expire_minutes: int = 60
    openai_api_key: str = ""
    openrouter_api_key: str = ""
    openai_api_base: str = ""
    openai_model: str = "gpt-4o-mini"
    ai_confidence_threshold: float = 0.65
    ai_memory_ttl_seconds: int = 900
    ai_high_risk_confirmation_required: bool = True
    ai_rate_limit_per_minute: int = 20

    @property
    def llm_api_key(self) -> str:
        return self.openai_api_key or self.openrouter_api_key

    @property
    def llm_api_base(self) -> str:
        if self.openai_api_base:
            return self.openai_api_base
        if self.openrouter_api_key:
            return "https://openrouter.ai/api/v1"
        return ""

    def cors_origin_list(self) -> list[str]:
        if not self.cors_origins:
            return []
        return [origin.strip() for origin in self.cors_origins.split(",") if origin.strip()]


@lru_cache
def get_settings() -> Settings:
    return Settings()


settings = get_settings()
