# Migration Plan - Smart Home (ESP32 Gateway)

## Phase 0 - Discovery
- Review Android legacy flows and device protocol (sta=, res=, drg=, dst=, HTTP usr*)
- Confirm ESP32 firmware behavior and endpoints
- Define Socket.IO event contract (ESP32 ↔ backend ↔ Flutter)

## Phase 1 - Backend Skeleton (FastAPI + Socket.IO)
- Create backend project layout and config management
- Add FastAPI ASGI app with python-socketio
- Implement logging, error handling, and environment settings

## Phase 2 - Data Model & Storage
- Define models: Users, Servers, Clients, SwitchModules, Status
- Create Pydantic schemas for REST + Socket.IO payloads
- Implement async DB layer (SQLAlchemy async + migrations)

## Phase 3 - REST APIs
- Auth: register/login (JWT)
- Server/Client CRUD
- SwitchModule CRUD + status reads
- Device config endpoints (ser/rem/cli/reg) for ESP32

## Phase 4 - Socket.IO Events
- Implement events: login, new user, command, status, response, staresult, upd client
- Add auth handshake for Socket.IO connections
- Persist and broadcast status updates

## Phase 5 - ESP32 Gateway (Arduino)
- Add Socket.IO client connection to backend
- Emit status/registration events in legacy-compatible format
- Receive command/status events and apply to hardware
- Implement reconnect + heartbeat

## Phase 6 - Flutter App Skeleton
- Create frontend structure: models, data, state, UI
- API client (REST + Socket.IO)
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
