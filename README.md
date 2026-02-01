# Smart Home Solutions

Full-stack migration of the legacy Android app into:
- **Backend**: FastAPI + WebSocket
- **Frontend**: Flutter (iOS/Android)
- **Gateway**: ESP32 (Arduino + WebSocketsClient)

## Repository layout
- `android-legacy/` legacy Android app (reference)
- `backend/` FastAPI backend
- `frontend/` Flutter app
- `socketio_contract.md` legacy-compatible WebSocket contract
- `backend/esp32_gateway.md` ESP32 gateway notes

## Backend (FastAPI)

### Setup
```bash
cd backend
python3 -m venv .venv
.venv/bin/python -m pip install -r requirements.txt
```

### Run locally
```bash
cd backend
.venv/bin/uvicorn app.main:app --reload --host 127.0.0.1 --port 8000
```

Health check:
```bash
curl http://127.0.0.1:8000/api/health
```

## Frontend (Flutter)

### Setup
```bash
cd frontend
flutter pub get
```

### Run on iPhone simulator (local backend)
```bash
flutter run --no-hot -d <SIMULATOR_ID> \
  --dart-define=SMART_HOME_BASE_URL=http://127.0.0.1:8000 \
  --dart-define=SMART_HOME_SOCKET_URL=http://127.0.0.1:8000
```

If you need hot reload on iOS, see the native assets note below.

### Production (hosted) config
Build the Flutter app against your hosted API domain:
```bash
flutter run -d <SIMULATOR_ID> \
  --dart-define=SMART_HOME_BASE_URL=https://api.sms.hebbit.tech \
  --dart-define=SMART_HOME_SOCKET_URL=https://api.sms.hebbit.tech \
  --dart-define=SMART_HOME_SOCKET_PATH=/ws
```

## Configuration (Flutter)
`frontend/lib/config/app_config.dart` reads these build-time values:
- `SMART_HOME_BASE_URL` (REST)
- `SMART_HOME_SOCKET_URL` (WebSocket, http/https is upgraded to ws/wss)
- `SMART_HOME_SOCKET_PATH` (default: `/ws`)

## ESP32 Gateway
See `backend/esp32_gateway.md` for the Arduino WebSocket skeleton and event mapping.

## Native assets note (iOS)
Current Flutter stable requires `SdkRoot` during native assets hook execution on hot reload.
Workaround: run with `--no-hot` (recommended for now). The issue is tracked in Flutter tooling.

## Parity validation
Use `parity_checklist.md` to validate behavior against the legacy app.

## Hosting (AWS Lightsail)
- Create a Lightsail Container Service once:
  - Name: `smart-home` (example)
  - Power: `nano`, Scale: `1`
  - Public endpoint: later via deployment (health: `/api/health`)
- Set GitHub Secrets for the workflow:
  - `AWS_REGION` (e.g. `us-east-1`)
  - `AWS_ROLE_TO_ASSUME` (OIDC role with Lightsail permissions)
  - `LIGHTSAIL_SERVICE_NAME` (e.g. `smart-home`)
  - `JWT_SECRET_KEY` (random string)
- On push to `main`, `.github/workflows/deploy-lightsail.yml` will:
  - Build `backend/` image
  - Push to Lightsail registry
  - Deploy to the container service with env vars & health checks

### Custom Domain (Lightsail)
- Domain: `https://api.sms.hebbit.tech`
- Lightsail → Networking → Create certificate → `api.sms.hebbit.tech`
- Add DNS validation CNAMEs in your DNS (Lightsail DNS zone or external)
- In the Container Service, attach the certificate to the public endpoint
- Backend CORS: ensure `CORS_ORIGINS=https://api.sms.hebbit.tech`
