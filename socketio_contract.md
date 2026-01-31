# Socket.IO Contract (Legacy-Compatible)

This document captures the event payloads used by the legacy Android app and
defines the contract to preserve behavior when migrating to FastAPI + ESP32
gateway + Flutter.

## Identifier rules
- server_id: the SSID-like ID stored with the "RSW-" prefix (example: "RSW-1234").
- dev_id: the client ID stored with the "RSW-" prefix (example: "RSW-5678").
- wire_dev_id: the ID sent over the wire without the "RSW-" prefix.

The legacy app uses dev_id.substring(4) and server_id.substring(4) when sending
Socket.IO payloads. The backend must preserve this behavior.

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
Ack:
- String message containing "success" on success.

### new user
Payload:
{
  "name": "user@example.com",
  "pword": "plaintext",
  "devID": "1234"
}
Ack:
- String message containing "success" on success.

### upd client
Payload:
{
  "name": "user@example.com",
  "pword": "plaintext",
  "devID": "1234"
}
Ack:
- String message (legacy app logs it).

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
Ack:
- JSON response echoing the command fields.

### status
Payload:
{
  "serverID": "1234",
  "devID": "5678",
  "comp": "Comp0"
}
Ack:
- JSON response echoing the request fields.

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

## ESP32 Gateway to Backend

The ESP32 must connect as a Socket.IO client and emit the same events that the
legacy server emitted to the Android app. The backend should accept these
payloads and persist them as status updates.

### staresult
Payload:
{
  "devID": "5678",
  "comp": "Comp0",
  "mod": 1,
  "stat": 1,
  "val": 800
}

### response
Payload:
{
  "devID": "5678",
  "comp": "Comp0",
  "mod": 1,
  "stat": 1,
  "val": 800
}

### register (ESP32 -> backend)
Use Socket.IO to replace local TCP register flow:
Payload:
{
  "serverID": "1234",
  "clientID": "5678",
  "ip": "192.168.4.20"
}

### register_status (ESP32 -> backend)
Payload:
{
  "devID": "5678",
  "comp": "Comp0",
  "mod": 1,
  "stat": 0,
  "val": 1000
}

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
