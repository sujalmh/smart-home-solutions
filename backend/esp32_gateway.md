# ESP32 Master Gateway (TCP + HTTP + Socket.IO)

The master ESP32 gateway sits on the LAN, listens for legacy TCP updates from
slave switchboards, and bridges them to the backend via Socket.IO. It also
receives backend commands and forwards them to slave ESPs over HTTP using the
legacy `usrcmd/usrini` protocol.

## Responsibilities
- Listen on TCP port `6000` for `sta=`, `res=`, `drg=`, `dst=` from slave ESPs.
- Parse and normalize legacy messages (wire IDs without `RSW-`).
- Maintain mapping of `slave_id -> ip` for HTTP routing.
- Emit `staresult`/`response` to backend only after real `sta=`/`res=`.
- Emit `register` on `drg=` and treat `dst=` as `staresult` (ignore `END`).
- Receive `command`/`status` from backend and forward to slaves via HTTP.

## Libraries
- `WiFi.h`
- `HTTPClient.h`
- `socket.io-client` (Arduino)
- `ArduinoJson`

## Connection details
- Backend URL: `http://YOUR_HOST:PORT`
- Socket.IO path: `/socket.io`
- TCP listen port: `6000`
- Slave HTTP port: `6000` (legacy firmware default)

## Event mapping

Inbound from backend:
- `command`: translate to `/?usrcmd=client;comp;mod;stat;val;`
- `status`: translate to `/?usrini=client;comp` (loop comps if missing)

Outbound to backend:
- `gateway_register`: `{ "serverID": "1234", "ip": "192.168.1.50" }`
- `register`: `{ "serverID": "1234", "clientID": "5678", "ip": "192.168.4.20" }`
- `staresult`: `{ "devID": "5678", "comp": "Comp0", "mod": 1, "stat": 1, "val": 800 }`
- `response`: `{ "devID": "5678", "comp": "Comp0", "mod": 1, "stat": 1, "val": 800 }`

Legacy TCP mapping:
- `sta=` -> `staresult`
- `res=` -> `response`
- `drg=` -> `register`
- `dst=` -> `staresult` (`END` ignored)

## Firmware
See `firmware/esp32_master_gateway/esp32_master_gateway.ino` for the Arduino
sketch.

## Notes
- Use wire IDs (no `RSW-`) on Socket.IO payloads.
- Backend is non-optimistic: Flutter gets updates only after real device state.
- For production, use `wss://` and validate certificates.
