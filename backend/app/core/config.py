from functools import lru_cache
from pydantic_settings import BaseSettings


class Settings(BaseSettings):
    app_name: str = "room-automation-backend"
    api_prefix: str = "/api"
    cors_origins: str = ""
    log_level: str = "INFO"
    database_url: str = "sqlite+aiosqlite:///./room_automation.db"
    jwt_secret_key: str = "change-me"
    jwt_algorithm: str = "HS256"
    jwt_access_token_expire_minutes: int = 60
    openai_api_key: str = ""
    openai_model: str = "gpt-4o-mini"
    ai_confidence_threshold: float = 0.65
    ai_memory_ttl_seconds: int = 900
    ai_high_risk_confirmation_required: bool = True
    ai_rate_limit_per_minute: int = 20

    def cors_origin_list(self) -> list[str]:
        if not self.cors_origins:
            return []
        return [origin.strip() for origin in self.cors_origins.split(",") if origin.strip()]


@lru_cache
def get_settings() -> Settings:
    return Settings()


settings = get_settings()
