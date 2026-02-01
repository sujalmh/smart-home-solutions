#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <HTTPClient.h>
#include <SocketIOclient.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* WIFI_SSID = "Nuron";
const char* WIFI_PASS = "mnbvcx12";

// Backend Socket.IO
const char* SOCKET_HOST = "api.sms.hebbit.tech"; // without scheme
const uint16_t SOCKET_PORT = 8000;
const char* SOCKET_PATH = "/socket.io";

// Master gateway identity (wire ID without RSW-)
const char* SERVER_ID = "1234";

// Legacy ports
const uint16_t TCP_PORT = 6000;
const uint16_t SLAVE_HTTP_PORT = 6000;

// Slave mapping
struct SlaveEntry {
  String id;
  String ip;
};

static const size_t MAX_SLAVES = 32;
SlaveEntry slaves[MAX_SLAVES];
size_t slaveCount = 0;

WiFiServer tcpServer(TCP_PORT);
SocketIOclient socketIO;

String normalizeId(const String& value) {
  if (value.startsWith("RSW-")) {
    return value.substring(4);
  }
  return value;
}

int toIntSafe(const String& value, int fallback) {
  if (value.length() == 0) {
    return fallback;
  }
  return value.toInt();
}

int splitTokens(const String& input, char delimiter, String* output, int maxTokens) {
  int count = 0;
  int start = 0;
  while (count < maxTokens) {
    int idx = input.indexOf(delimiter, start);
    if (idx == -1) {
      output[count++] = input.substring(start);
      break;
    }
    output[count++] = input.substring(start, idx);
    start = idx + 1;
  }
  return count;
}

String findSlaveIp(const String& id) {
  for (size_t i = 0; i < slaveCount; i++) {
    if (slaves[i].id == id) {
      return slaves[i].ip;
    }
  }
  return String();
}

void upsertSlave(const String& id, const String& ip) {
  if (id.length() == 0 || ip.length() == 0) {
    return;
  }
  for (size_t i = 0; i < slaveCount; i++) {
    if (slaves[i].id == id) {
      slaves[i].ip = ip;
      return;
    }
  }
  if (slaveCount < MAX_SLAVES) {
    slaves[slaveCount++] = {id, ip};
  }
}

bool sendHttpGet(const String& ip, const String& path) {
  if (ip.length() == 0) {
    return false;
  }
  WiFiClient client;
  HTTPClient http;
  String url = "http://" + ip + ":" + String(SLAVE_HTTP_PORT) + path;
  if (!http.begin(client, url)) {
    return false;
  }
  int code = http.GET();
  http.end();
  return code > 0 && code < 400;
}

void emitEvent(const char* eventName, JsonDocument& doc) {
  String payload;
  serializeJson(doc, payload);
  String message = "[\"" + String(eventName) + "\"," + payload + "]";
  socketIO.sendEVENT(message);
}

void emitGatewayRegister() {
  DynamicJsonDocument doc(256);
  doc["serverID"] = SERVER_ID;
  doc["ip"] = WiFi.localIP().toString();
  emitEvent("gateway_register", doc);
}

void emitRegister(const String& clientId, const String& ip) {
  DynamicJsonDocument doc(256);
  doc["serverID"] = SERVER_ID;
  doc["clientID"] = clientId;
  doc["ip"] = ip;
  emitEvent("register", doc);
}

void emitStaResult(const String& devId, const String& comp, int mod, int stat, int val) {
  DynamicJsonDocument doc(256);
  doc["devID"] = devId;
  doc["comp"] = comp;
  doc["mod"] = mod;
  doc["stat"] = stat;
  doc["val"] = val;
  emitEvent("staresult", doc);
}

void emitResponse(const String& devId, const String& comp, int mod, int stat, int val) {
  DynamicJsonDocument doc(256);
  doc["devID"] = devId;
  doc["comp"] = comp;
  doc["mod"] = mod;
  doc["stat"] = stat;
  doc["val"] = val;
  emitEvent("response", doc);
}

void handleTcpLine(const String& line, const String& remoteIp) {
  if (line.startsWith("drg=")) {
    String payload = line.substring(4);
    String tokens[2];
    int count = splitTokens(payload, ';', tokens, 2);
    if (count >= 2) {
      String clientId = normalizeId(tokens[0]);
      String ip = tokens[1];
      upsertSlave(clientId, ip);
      emitRegister(clientId, ip);
    }
    return;
  }

  if (line.startsWith("sta=") || line.startsWith("res=") || line.startsWith("dst=")) {
    String payload = line.substring(4);
    String tokens[5];
    int count = splitTokens(payload, ';', tokens, 5);
    if (count < 2) {
      return;
    }

    String clientId = normalizeId(tokens[0]);
    if (clientId.indexOf("END") >= 0) {
      return;
    }

    if (remoteIp.length() > 0) {
      upsertSlave(clientId, remoteIp);
    }

    if (count < 5) {
      return;
    }

    String comp = tokens[1];
    int mod = toIntSafe(tokens[2], 0);
    int stat = toIntSafe(tokens[3], 0);
    int val = toIntSafe(tokens[4], 0);
    if (val > 1000) {
      val = 1000;
    }

    if (line.startsWith("res=")) {
      emitResponse(clientId, comp, mod, stat, val);
    } else {
      emitStaResult(clientId, comp, mod, stat, val);
    }
  }
}

void handleTcpClients() {
  WiFiClient client = tcpServer.available();
  if (!client) {
    return;
  }

  client.setTimeout(50);
  String remoteIp = client.remoteIP().toString();
  String line = client.readStringUntil('\n');
  line.trim();
  if (line.length() > 0) {
    handleTcpLine(line, remoteIp);
  }
  client.stop();
}

void sendCommandToSlave(const String& devId, const String& comp, int mod, int stat, int val) {
  String ip = findSlaveIp(devId);
  if (ip.length() == 0) {
    return;
  }
  String path = "/?usrcmd=" + devId + ";" + comp + ";" + String(mod) + ";" +
                String(stat) + ";" + String(val) + ";";
  sendHttpGet(ip, path);
}

void sendStatusToSlave(const String& devId, const String& comp) {
  String ip = findSlaveIp(devId);
  if (ip.length() == 0) {
    return;
  }
  String path = "/?usrini=" + devId + ";" + comp;
  sendHttpGet(ip, path);
}

void handleCommand(JsonObject data) {
  String serverId = normalizeId(String(data["serverID"] | ""));
  if (serverId.length() > 0 && serverId != SERVER_ID) {
    return;
  }
  String devId    = normalizeId(String(data["devID"] | ""));
  String comp     = String(data["comp"] | "");
  int mod = data["mod"] | 0;
  int stat = data["stat"] | 0;
  int val = data["val"] | 0;
  sendCommandToSlave(devId, comp, mod, stat, val);
}

void handleStatus(JsonObject data) {
  String serverId = normalizeId(String(data["serverID"] | ""));
  if (serverId.length() > 0 && serverId != SERVER_ID) {
    return;
  }
  String devId    = normalizeId(String(data["devID"] | ""));
  String comp     = String(data["comp"] | "");
  if (comp.length() == 0) {
    sendStatusToSlave(devId, "Comp0");
    sendStatusToSlave(devId, "Comp1");
    sendStatusToSlave(devId, "Comp2");
  } else {
    sendStatusToSlave(devId, comp);
  }
}

void onSocketEvent(socketIOmessageType_t type, uint8_t* payload, size_t length) {
  if (type == sIOtype_CONNECT) {
    emitGatewayRegister();
    return;
  }
  if (type != sIOtype_EVENT) {
    return;
  }

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error) {
    return;
  }
  if (!doc.is<JsonArray>()) {
    return;
  }
  const char* eventName = doc[0];
  JsonObject data = doc[1].as<JsonObject>();
  if (!eventName || data.isNull()) {
    return;
  }

  if (strcmp(eventName, "command") == 0) {
    handleCommand(data);
  } else if (strcmp(eventName, "status") == 0) {
    handleStatus(data);
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  tcpServer.begin();
  socketIO.begin(SOCKET_HOST, SOCKET_PORT, SOCKET_PATH);
  socketIO.onEvent(onSocketEvent);
  socketIO.setReconnectInterval(5000);
}

void loop() {
  socketIO.loop();
  handleTcpClients();
}
