# ESP32 Master Gateway (TCP + HTTP + WebSocket)

The master ESP32 gateway sits on the LAN, listens for legacy TCP updates from
slave switchboards, and bridges them to the backend via WebSockets. It also
receives backend commands and forwards them to slave ESPs over HTTP using the
legacy `usrcmd/usrini` protocol.

## Responsibilities
- Listen on TCP port `6000` for `sta=`, `res=`, `drg=`, `dst=` from slave ESPs.
- Listen on UDP port `6000` for `drg=` discovery broadcasts.
- Parse and normalize legacy messages (wire IDs without `RSW-`).
- Maintain mapping of `slave_id -> ip` for HTTP routing.
- Require binding before accepting `drg=` updates (discover only).
- Emit `staresult`/`response` to backend only after real `sta=`/`res=`.
- Emit `register` on `drg=` and treat `dst=` as `staresult` (ignore `END`).
- Receive `command`/`status` from backend and forward to slaves via HTTP.
 - Accept `bind_slave` from backend to allow a slave ID.

## Libraries
- `WiFi.h`
- `HTTPClient.h`
- `WebSocketsClient` (ArduinoWebSockets)
- `ArduinoJson`

## Connection details
- Backend URL: `https://YOUR_HOST`
- WebSocket path: `/ws`
- TCP listen port: `6000`
- Slave HTTP port: `6000` (legacy firmware default)

## Event mapping

Inbound from backend:
- `command`: translate to `/?usrcmd=client;comp;mod;stat;val;`
- `status`: translate to `/?usrini=client;comp` (loop comps if missing)

Outbound to backend:
- `gateway_register`: `{ "serverID": "1234", "ip": "192.168.1.50" }`
- `register`: `{ "serverID": "1234", "clientID": "5678", "ip": "192.168.4.20" }`
- `slave_seen`: `{ "serverID": "1234", "clientID": "5678", "ip": "192.168.4.20" }`
- `staresult`: `{ "devID": "5678", "comp": "Comp0", "mod": 1, "stat": 1, "val": 800 }`
- `response`: `{ "devID": "5678", "comp": "Comp0", "mod": 1, "stat": 1, "val": 800 }`

Inbound from backend:
- `bind_slave`: `{ "serverID": "1234", "clientID": "5678" }`
- `unbind_slave`: `{ "serverID": "1234", "clientID": "5678" }`

Binding behavior:
- `drg=` always updates discovery (`slave_seen`).
- The master only registers/binds after the app sends `bind_slave`.

Legacy TCP mapping:
- `sta=` -> `staresult`
- `res=` -> `response`
- `drg=` -> `register`
- `dst=` -> `staresult` (`END` ignored)

## Firmware
See `firmware/esp32_master_gateway/esp32_master_gateway.ino` for the Arduino
sketch.

## Notes
- Use wire IDs (no `RSW-`) on WebSocket payloads.
- Backend is non-optimistic: Flutter gets updates only after real device state.
- For production, use `wss://` and validate certificates.
