# Migration Plan - Smart Home (ESP32 Gateway)

## Phase 0 - Discovery
- Review Android legacy flows and device protocol (sta=, res=, drg=, dst=, HTTP usr*)
- Confirm ESP32 firmware behavior and endpoints
- Define WebSocket event contract (ESP32 ↔ backend ↔ Flutter)

## Phase 1 - Backend Skeleton (FastAPI + WebSocket)
- Create backend project layout and config management
- Add FastAPI ASGI app with WebSocket endpoint
- Implement logging, error handling, and environment settings

## Phase 2 - Data Model & Storage
- Define models: Users, Servers, Clients, SwitchModules, Status
- Create Pydantic schemas for REST + WebSocket payloads
- Implement async DB layer (SQLAlchemy async + migrations)

## Phase 3 - REST APIs
- Auth: register/login (JWT)
- Server/Client CRUD
- SwitchModule CRUD + status reads
- Device config endpoints (ser/rem/cli/reg) for ESP32

## Phase 4 - WebSocket Events
- Implement events: command, status, response, staresult, register
- Add auth handshake for WebSocket connections
- Persist and broadcast status updates

## Phase 5 - ESP32 Gateway (Arduino)
- Add WebSocket client connection to backend
- Emit status/registration events in legacy-compatible format
- Receive command/status events and apply to hardware
- Implement reconnect + heartbeat

## Phase 6 - Flutter App Skeleton
- Create frontend structure: models, data, state, UI
- API client (REST + WebSocket)
- State management (Riverpod/BLoC)

## Phase 7 - UI Parity
- Rebuild screens: Room, Select, Config, Register, RemoteLogin, RoomSwitches, Switch
- Wire all actions to backend APIs
- WebSocket status stream integration

## Phase 8 - Parity Validation
- Verify device registration flow
- Verify switch control + dimming behavior
- Verify real-time status updates
- Validate auth/login behavior

## Phase 9 - Packaging & Docs
- Add setup docs for backend, ESP32, Flutter
- Include environment configuration and deployment notes
