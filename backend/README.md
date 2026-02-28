# Backend

## Run locally

```bash
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
uvicorn app.main:app --reload
```

## Environment
- Copy `.env.example` to `.env` and adjust values.
- For AI via OpenRouter, set:
  - `OPENROUTER_API_KEY=<your_key>`
  - `OPENAI_API_BASE=https://openrouter.ai/api/v1` (optional if omitted, backend defaults this when `OPENROUTER_API_KEY` is set)
