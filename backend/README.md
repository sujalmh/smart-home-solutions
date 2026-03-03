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

## Cloud Run deploy notes
- The GitHub workflow `.github/workflows/cloud-run-deploy.yml` now applies reliable defaults:
  - `min instances=1`, `max instances=20`, `concurrency=80`, `timeout=120s`, `cpu=1`, `memory=512Mi`.
- Override with repository variables when needed:
  - `CLOUD_RUN_MIN_INSTANCES`, `CLOUD_RUN_MAX_INSTANCES`, `CLOUD_RUN_CONCURRENCY`, `CLOUD_RUN_TIMEOUT`, `CLOUD_RUN_CPU`, `CLOUD_RUN_MEMORY`.
- To avoid request aborts due to no available instance under load, keep `CLOUD_RUN_MIN_INSTANCES` >= `1` and raise `CLOUD_RUN_MAX_INSTANCES` based on traffic.
