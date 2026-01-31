# Parity Validation Checklist

Use this checklist to confirm the Flutter + FastAPI migration preserves the
legacy Android behavior.

## Backend (FastAPI + Socket.IO)
- Health endpoint returns OK: `GET /api/health`
- Auth register/login works with existing users (password hash verified)
- Socket.IO events accepted: login, new user, upd client, command, status
- command updates switch module state in DB and emits response
- status emits staresult for requested client/module
- staresult/response ingested from ESP32 and persisted
- Client creation auto-seeds Comp0/Comp1/Comp2 with mode=1, status=0, value=1000
- Device config endpoints accept payloads:
  - /api/devices/{server_id}/config/server
  - /api/devices/{server_id}/config/remote
  - /api/devices/{server_id}/config/client
- Device register endpoint persists server if missing

## ESP32 Gateway
- Socket.IO connection stays alive and reconnects
- Receives command and applies hardware state
- Emits response with devID/comp/mod/stat/val
- Responds to status by emitting staresult
- (Optional) emits register and register_status on first setup

## Flutter App
- Home menu navigates to Remote Login, Local Register, Select Device, Switch Modules
- Remote Login:
  - Login success routes to Room Switches
  - Register success shows confirmation
- Local Register:
  - Register sends /api/devices/{server_id}/register
- Select Device:
  - Routes to Config screen with device ID
- Config:
  - Server mode sends config/server and config/remote
  - Client mode sends config/client
  - Both mode sends server + client configs
- Room Switches:
  - Lists clients for selected server
  - Navigates to Switch screen
- Switch screen:
  - Loads modules from /api/clients/{client_id}/modules
  - Manual/Auto toggles update via command
  - Power toggle updates status via command
  - Slider sends val updates
  - Refresh triggers status request
  - Socket updates apply to UI in real-time

## Regression items from legacy
- devID and serverID handling: wire IDs are without RSW- prefix
- Mode mapping: 0 = Manual, 1 = Auto
- Status mapping: 0 = Off, 1 = On
- Value clamp: 0..1000
