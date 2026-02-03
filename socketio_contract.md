# WebSocket Contract (Legacy-Compatible)

This document captures the event payloads used by the legacy Android app and
defines the contract to preserve behavior when migrating to FastAPI + ESP32
gateway + Flutter over WebSockets.

## WebSocket envelope
All WebSocket messages use a JSON envelope:
{
  "event": "event_name",
  "data": { ... }
}

The payloads below refer to the `data` object for each event.

Connection endpoint: `wss://<host>/ws`

## Identifier rules
- server_id: the SSID-like ID stored with the "RSW-" prefix (example: "RSW-1234").
- dev_id: the client ID stored with the "RSW-" prefix (example: "RSW-5678").
- wire_dev_id: the ID sent over the wire without the "RSW-" prefix.

The legacy app uses dev_id.substring(4) and server_id.substring(4) when sending
payloads. The backend must preserve this behavior.

## Shared JSON fields
- serverID: wire_dev_id for the server (string).
- devID: wire_dev_id for the client device (string).
- comp: component id, "Comp0" | "Comp1" | "Comp2" (string).
- mod: mode, 0 = Manual, 1 = Auto (int).
- stat: state, 0 = Off, 1 = On (int).
- val: value (int, clamp 0-1000).

## Client (Flutter) to Backend

### login
Payload:
{
  "name": "user@example.com",
  "pword": "plaintext",
  "devID": "1234"
}

### new user
Payload:
{
  "name": "user@example.com",
  "pword": "plaintext",
  "devID": "1234"
}

### upd client
Payload:
{
  "name": "user@example.com",
  "pword": "plaintext",
  "devID": "1234"
}

### command
Payload:
{
  "serverID": "1234",
  "devID": "5678",
  "comp": "Comp0",
  "mod": 1,
  "stat": 1,
  "val": 800
}

### status
Payload:
{
  "serverID": "1234",
  "devID": "5678",
  "comp": "Comp0"
}
Notes:
- Backend forwards `command`/`status` to the master gateway only.
- Flutter receives `response`/`staresult` only after real `res=`/`sta=` from slaves.

### bind_slave
Payload:
{
  "serverID": "1234",
  "clientID": "5678"
}
Meaning:
- Requests the master gateway to bind/allow a slave ID.

### unbind_slave
Payload:
{
  "serverID": "1234",
  "clientID": "5678"
}
Meaning:
- Requests the master gateway to drop a bound slave ID.

## Backend to Client (Flutter)

### response
Payload:
{
  "devID": "5678",
  "comp": "Comp0",
  "mod": 1,
  "stat": 1,
  "val": 800
}
Meaning:
- Response for a command event.

### staresult
Payload:
{
  "devID": "5678",
  "comp": "Comp0",
  "mod": 1,
  "stat": 1,
  "val": 800
}
Meaning:
- Status response for status requests.

## Master ESP32 Gateway to Backend

The master ESP32 gateway connects over WebSocket (`/ws`), listens for TCP from
slave switchboards, and forwards real device updates. The backend must only
emit `response` and `staresult` to Flutter after receiving real `res=`/`sta=`.

### gateway_register (master -> backend)
Payload:
{
  "serverID": "1234",
  "ip": "192.168.1.50"
}
Meaning:
- Registers the master gateway in a per-server room for routing.

### staresult (master -> backend)
Payload:
{
  "devID": "5678",
  "comp": "Comp0",
  "mod": 1,
  "stat": 1,
  "val": 800
}

### response (master -> backend)
Payload:
{
  "devID": "5678",
  "comp": "Comp0",
  "mod": 1,
  "stat": 1,
  "val": 800
}

### register (master -> backend)
Use WebSocket to replace local TCP register flow (from `drg=`):
Payload:
{
  "serverID": "1234",
  "clientID": "5678",
  "ip": "192.168.4.20"
}
Notes:
- `dst=` registration status lines are forwarded as `staresult`.
- `END` markers are ignored except for local completion tracking.

### slave_seen (master -> backend)
Payload:
{
  "serverID": "1234",
  "clientID": "5678",
  "ip": "192.168.4.20"
}
Meaning:
- Reports a slave seen on the LAN even if unbound.

## Backend to Master ESP32 Gateway

### bind_slave
Payload:
{
  "serverID": "1234",
  "clientID": "5678"
}
Meaning:
- Allows/binds the slave to the master. The master should accept `drg=` from the slave.

## Backend to ESP32 Gateway

### command
Payload:
{
  "serverID": "1234",
  "devID": "5678",
  "comp": "Comp0",
  "mod": 1,
  "stat": 1,
  "val": 800
}

### status
Payload:
{
  "serverID": "1234",
  "devID": "5678",
  "comp": "Comp0"
}

## Legacy HTTP/TCP reference (for ESP32 firmware mapping)

HTTP GET (legacy, local only):
- /?ser= (server set)
- /?rem=ssid;pwd; (remote config)
- /?cli=ssid;pwd;ip (client config)
- /?reg= (register)
- /?usrser= (server set)
- /?usrrem=ssid;pwd; (remote config)
- /?usrcli=ssid;pwd;ip (client config)
- /?usrreg= (register)
- /?usrcmd=client;comp;mod;stat;val;
- /?usrini=client;comp

TCP 6000 messages (legacy, local only):
- sta=client;comp;mod;stat;val
- res=client;comp;mod;stat;val
- drg=client;ip
- dst=client;comp;mod;stat;val (END indicates done)
