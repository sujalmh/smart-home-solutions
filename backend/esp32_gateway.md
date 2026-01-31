# ESP32 Gateway (Arduino + socket.io-client)

This gateway replaces the legacy phone-as-gateway behavior. The ESP32 connects
outbound to the remote FastAPI Socket.IO server and relays status and commands.

## Responsibilities
- Connect to Wi-Fi and maintain Socket.IO connection to the remote host.
- Receive `command` and `status` events from backend.
- Emit `response` and `staresult` events with component state.
- Emit `register` and `register_status` when a device is added (optional).

## Library
- `socket.io-client` (Arduino)
- `WebSocketsClient` (dependency of socket.io-client)

## Connection details
- URL: `http://YOUR_HOST:PORT`
- Path: `/socket.io` (matches backend `SOCKETIO_PATH`)
- Transport: websocket (recommended)

## Event mapping
Inbound from backend:
- `command`: apply state to hardware, then emit `response` with new state
- `status`: emit `staresult` with current state

Outbound to backend:
- `response`: result of a `command`
- `staresult`: periodic or requested status
- `register`: initial registration (optional)
- `register_status`: send initial module state (optional)

Payload fields match `socketio_contract.md`.

## Minimal sketch skeleton

```cpp
#include <WiFi.h>
#include <SocketIOclient.h>

SocketIOclient socketIO;

const char* WIFI_SSID = "YOUR_WIFI";
const char* WIFI_PASS = "YOUR_PASS";

const char* HOST = "your.remote.host";
const uint16_t PORT = 8000;
const char* SOCKET_PATH = "/socket.io";

String serverId = "1234"; // without RSW-
String devId = "5678";    // without RSW-

void emitResponse(const String& comp, int mod, int stat, int val) {
  String payload = "[\"response\",{" \
    "\"devID\":\"" + devId + "\"," \
    "\"comp\":\"" + comp + "\"," \
    "\"mod\":" + String(mod) + "," \
    "\"stat\":" + String(stat) + "," \
    "\"val\":" + String(val) + "}]";
  socketIO.sendEVENT(payload);
}

void emitStaResult(const String& comp, int mod, int stat, int val) {
  String payload = "[\"staresult\",{" \
    "\"devID\":\"" + devId + "\"," \
    "\"comp\":\"" + comp + "\"," \
    "\"mod\":" + String(mod) + "," \
    "\"stat\":" + String(stat) + "," \
    "\"val\":" + String(val) + "}]";
  socketIO.sendEVENT(payload);
}

void onSocketEvent(socketIOmessageType_t type, uint8_t* payload, size_t length) {
  if (type != sIOtype_EVENT) {
    return;
  }

  String msg = String((char*)payload);
  if (msg.indexOf("\"command\"") >= 0) {
    // TODO: parse JSON; update hardware state
    // Example: comp="Comp0", mod=1, stat=1, val=800
    emitResponse("Comp0", 1, 1, 800);
  } else if (msg.indexOf("\"status\"") >= 0) {
    // TODO: parse JSON; emit current state
    emitStaResult("Comp0", 1, 1, 800);
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  socketIO.begin(HOST, PORT, SOCKET_PATH);
  socketIO.onEvent(onSocketEvent);
  socketIO.setReconnectInterval(5000);
}

void loop() {
  socketIO.loop();
}
```

## Notes
- For production, use `wss://` and validate certificates.
- Replace placeholder JSON parsing with a small JSON parser (ArduinoJson).
- Keep `devID` and `serverID` consistent with backend expectations.
