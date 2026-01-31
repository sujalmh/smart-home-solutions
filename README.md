# Smart Home Solutions

Full-stack migration of the legacy Android app into:
- **Backend**: FastAPI + Socket.IO
- **Frontend**: Flutter (iOS/Android)
- **Gateway**: ESP32 (Arduino + Socket.IO client)

## Repository layout
- `android-legacy/` legacy Android app (reference)
- `backend/` FastAPI backend
- `frontend/` Flutter app
- `socketio_contract.md` legacy-compatible event contract
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
  --dart-define=SMART_HOME_SOCKET_PATH=/socket.io
```

## Configuration (Flutter)
`frontend/lib/config/app_config.dart` reads these build-time values:
- `SMART_HOME_BASE_URL` (REST)
- `SMART_HOME_SOCKET_URL` (Socket.IO)
- `SMART_HOME_SOCKET_PATH` (default: `/socket.io`)

## ESP32 Gateway
See `backend/esp32_gateway.md` for the Arduino Socket.IO skeleton and event mapping.

## Native assets note (iOS)
Current Flutter stable requires `SdkRoot` during native assets hook execution on hot reload.
Workaround: run with `--no-hot` (recommended for now). The issue is tracked in Flutter tooling.

## Parity validation
Use `parity_checklist.md` to validate behavior against the legacy app.

## Custom Domain (App Runner)
- Domain: `https://api.sms.hebbit.tech`
- App Runner → Custom domain → add `api.sms.hebbit.tech`
- Complete DNS validation (CNAMEs) in your DNS host (Route 53 or external)
- Attach/issue ACM certificate in App Runner
- Backend CORS: set `CORS_ORIGINS=https://api.sms.hebbit.tech` in the service env
