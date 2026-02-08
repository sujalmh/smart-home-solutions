#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <HTTPClient.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* WIFI_SSID = "Nuron";
const char* WIFI_PASS = "mnbvcx12";

// Backend WebSocket
const char* SOCKET_HOST = "api.sms.hebbit.tech"; // without scheme
const uint16_t SOCKET_PORT = 443;
const char* SOCKET_PATH = "/ws";
const bool SOCKET_INSECURE = true;
const char* SOCKET_CA_CERT = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFYjCCBEqgAwIBAgIQd70NbNs2+RrqIQ/E8FjTDTANBgkqhkiG9w0BAQsFADBX
MQswCQYDVQQGEwJCRTEZMBcGA1UEChMQR2xvYmFsU2lnbiBudi1zYTEQMA4GA1UE
CxMHUm9vdCBDQTEbMBkGA1UEAxMSR2xvYmFsU2lnbiBSb290IENBMB4XDTIwMDYx
OTAwMDA0MloXDTI4MDEyODAwMDA0MlowRzELMAkGA1UEBhMCVVMxIjAgBgNVBAoT
GUdvb2dsZSBUcnVzdCBTZXJ2aWNlcyBMTEMxFDASBgNVBAMTC0dUUyBSb290IFIx
MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAthECix7joXebO9y/lD63
ladAPKH9gvl9MgaCcfb2jH/76Nu8ai6Xl6OMS/kr9rH5zoQdsfnFl97vufKj6bwS
iV6nqlKr+CMny6SxnGPb15l+8Ape62im9MZaRw1NEDPjTrETo8gYbEvs/AmQ351k
KSUjB6G00j0uYODP0gmHu81I8E3CwnqIiru6z1kZ1q+PsAewnjHxgsHA3y6mbWwZ
DrXYfiYaRQM9sHmklCitD38m5agI/pboPGiUU+6DOogrFZYJsuB6jC511pzrp1Zk
j5ZPaK49l8KEj8C8QMALXL32h7M1bKwYUH+E4EzNktMg6TO8UpmvMrUpsyUqtEj5
cuHKZPfmghCN6J3Cioj6OGaK/GP5Afl4/Xtcd/p2h/rs37EOeZVXtL0m79YB0esW
CruOC7XFxYpVq9Os6pFLKcwZpDIlTirxZUTQAs6qzkm06p98g7BAe+dDq6dso499
iYH6TKX/1Y7DzkvgtdizjkXPdsDtQCv9Uw+wp9U7DbGKogPeMa3Md+pvez7W35Ei
Eua++tgy/BBjFFFy3l3WFpO9KWgz7zpm7AeKJt8T11dleCfeXkkUAKIAf5qoIbap
sZWwpbkNFhHax2xIPEDgfg1azVY80ZcFuctL7TlLnMQ/0lUTbiSw1nH69MG6zO0b
9f6BQdgAmD06yK56mDcYBZUCAwEAAaOCATgwggE0MA4GA1UdDwEB/wQEAwIBhjAP
BgNVHRMBAf8EBTADAQH/MB0GA1UdDgQWBBTkrysmcRorSCeFL1JmLO/wiRNxPjAf
BgNVHSMEGDAWgBRge2YaRQ2XyolQL30EzTSo//z9SzBgBggrBgEFBQcBAQRUMFIw
JQYIKwYBBQUHMAGGGWh0dHA6Ly9vY3NwLnBraS5nb29nL2dzcjEwKQYIKwYBBQUH
MAKGHWh0dHA6Ly9wa2kuZ29vZy9nc3IxL2dzcjEuY3J0MDIGA1UdHwQrMCkwJ6Al
oCOGIWh0dHA6Ly9jcmwucGtpLmdvb2cvZ3NyMS9nc3IxLmNybDA7BgNVHSAENDAy
MAgGBmeBDAECATAIBgZngQwBAgIwDQYLKwYBBAHWeQIFAwIwDQYLKwYBBAHWeQIF
AwMwDQYJKoZIhvcNAQELBQADggEBADSkHrEoo9C0dhemMXoh6dFSPsjbdBZBiLg9
NR3t5P+T4Vxfq7vqfM/b5A3Ri1fyJm9bvhdGaJQ3b2t6yMAYN/olUazsaL+yyEn9
WprKASOshIArAoyZl+tJaox118fessmXn1hIVw41oeQa1v1vg4Fv74zPl6/AhSrw
9U5pCZEt4Wi4wStz6dTZ/CLANx8LZh1J7QJVj2fhMtfTJr9w4z30Z209fOU0iOMy
+qduBmpvvYuR7hZL6Dupszfnw0Skfths18dG9ZKb59UhvmaSGZRVbNQpsg3BZlvi
d0lIKO2d1xozclOzgjXPYovJJIultzkMu34qQb9Sz/yilrbCgj8=
-----END CERTIFICATE-----
)EOF";

// Master gateway identity (wire ID without RSW-)
const char* SERVER_ID = "1234";

// Legacy ports
const uint16_t TCP_PORT = 6000;
const uint16_t SLAVE_HTTP_PORT = 80;
const bool LOG_VERBOSE = true;
const bool REQUIRE_BINDING = true;
const uint16_t UDP_DISCOVERY_PORT = 6000;
const unsigned long SEEN_TTL_MS = 30000;

// Slave mapping
struct SlaveEntry {
  String id;
  String ip;
};

static const size_t MAX_SLAVES = 32;
SlaveEntry slaves[MAX_SLAVES];
size_t slaveCount = 0;

static const size_t MAX_PENDING = 32;
String pendingSlaves[MAX_PENDING];
size_t pendingCount = 0;

static const size_t MAX_SEEN = 32;
String seenIds[MAX_SEEN];
String seenIps[MAX_SEEN];
unsigned long seenAt[MAX_SEEN];
size_t seenCount = 0;

WiFiServer tcpServer(TCP_PORT);
WebSocketsClient webSocket;
WiFiUDP udp;

String normalizeId(const String& value) {
  if (value.startsWith("RSW-")) {
    return value.substring(4);
  }
  return value;
}

void logLine(const String& message) {
  if (!LOG_VERBOSE) {
    return;
  }
  Serial.println(message);
}

const char* webSocketTypeName(WStype_t type) {
  switch (type) {
    case WStype_DISCONNECTED:
      return "DISCONNECTED";
    case WStype_CONNECTED:
      return "CONNECTED";
    case WStype_TEXT:
      return "TEXT";
    case WStype_BIN:
      return "BIN";
    case WStype_ERROR:
      return "ERROR";
    case WStype_PING:
      return "PING";
    case WStype_PONG:
      return "PONG";
    default:
      return "UNKNOWN";
  }
}

String payloadToString(uint8_t* payload, size_t length) {
  String out;
  for (size_t i = 0; i < length; i++) {
    out += static_cast<char>(payload[i]);
  }
  return out;
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

String findSeenIp(const String& id) {
  for (size_t i = 0; i < seenCount; i++) {
    if (seenIds[i] == id) {
      return seenIps[i];
    }
  }
  return String();
}

bool isKnownSlave(const String& id) {
  for (size_t i = 0; i < slaveCount; i++) {
    if (slaves[i].id == id) {
      return true;
    }
  }
  return false;
}

bool isPendingSlave(const String& id) {
  for (size_t i = 0; i < pendingCount; i++) {
    if (pendingSlaves[i] == id) {
      return true;
    }
  }
  return false;
}

void addPendingSlave(const String& id) {
  if (id.length() == 0 || isPendingSlave(id)) {
    return;
  }
  if (pendingCount < MAX_PENDING) {
    pendingSlaves[pendingCount++] = id;
  }
}

void removePendingSlave(const String& id) {
  for (size_t i = 0; i < pendingCount; i++) {
    if (pendingSlaves[i] == id) {
      pendingSlaves[i] = pendingSlaves[pendingCount - 1];
      pendingCount--;
      return;
    }
  }
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
    logLine("HTTP skip: missing slave IP");
    return false;
  }
  WiFiClient client;
  HTTPClient http;
  String url = "http://" + ip + ":" + String(SLAVE_HTTP_PORT) + path;
  logLine("HTTP GET -> " + url);
  http.setTimeout(3000);
  if (!http.begin(client, url)) {
    logLine("HTTP begin failed");
    return false;
  }
  int code = http.GET();
  if (code <= 0) {
    logLine("HTTP error: " + http.errorToString(code));
  } else {
    logLine("HTTP status: " + String(code));
  }
  http.end();
  return code > 0 && code < 400;
}
  
void emitEvent(const char* eventName, const JsonDocument& doc) {
  DynamicJsonDocument envelope(512);
  envelope["event"] = eventName;
  envelope["data"].set(doc.as<JsonVariantConst>());
  String message;
  serializeJson(envelope, message);
  webSocket.sendTXT(message);
}

void emitGatewayRegister() {
  DynamicJsonDocument doc(256);
  doc["serverID"] = SERVER_ID;
  doc["ip"] = WiFi.localIP().toString();
  logLine("WebSocket emit gateway_register server=" + String(SERVER_ID));
  emitEvent("gateway_register", doc);
}

void emitRegister(const String& clientId, const String& ip) {
  DynamicJsonDocument doc(256);
  doc["serverID"] = SERVER_ID;
  doc["clientID"] = clientId;
  doc["ip"] = ip;
  logLine("WebSocket emit register client=" + clientId + " ip=" + ip);
  emitEvent("register", doc);
}

void emitSlaveSeen(const String& clientId, const String& ip) {
  DynamicJsonDocument doc(256);
  doc["serverID"] = SERVER_ID;
  doc["clientID"] = clientId;
  doc["ip"] = ip;
  logLine("WebSocket emit slave_seen client=" + clientId + " ip=" + ip);
  emitEvent("slave_seen", doc);
}

void recordSeen(const String& clientId, const String& ip, bool forceEmit) {
  if (clientId.length() == 0) {
    return;
  }
  unsigned long now = millis();
  for (size_t i = 0; i < seenCount; i++) {
    if (seenIds[i] == clientId) {
      seenIps[i] = ip;
      bool shouldEmit = forceEmit || (now - seenAt[i] >= SEEN_TTL_MS);
      seenAt[i] = now;
      if (shouldEmit && webSocket.isConnected()) {
        emitSlaveSeen(clientId, ip);
      }
      return;
    }
  }
  if (seenCount < MAX_SEEN) {
    seenIds[seenCount] = clientId;
    seenIps[seenCount] = ip;
    seenAt[seenCount] = now;
    seenCount++;
  }
  if (webSocket.isConnected()) {
    emitSlaveSeen(clientId, ip);
  }
}

void flushSeen() {
  if (!webSocket.isConnected()) {
    return;
  }
  for (size_t i = 0; i < seenCount; i++) {
    emitSlaveSeen(seenIds[i], seenIps[i]);
  }
}

void handleUdpDiscovery() {
  int packetSize = udp.parsePacket();
  if (packetSize <= 0) {
    return;
  }
  char buffer[128];
  int len = udp.read(buffer, sizeof(buffer) - 1);
  if (len <= 0) {
    return;
  }
  buffer[len] = '\0';
  String line = String(buffer);
  line.trim();
  if (!line.startsWith("drg=")) {
    return;
  }
  String payload = line.substring(4);
  String tokens[2];
  int count = splitTokens(payload, ';', tokens, 2);
  if (count < 2) {
    return;
  }
  String clientId = normalizeId(tokens[0]);
  String ip = tokens[1];
  recordSeen(clientId, ip, false);
  if (isPendingSlave(clientId)) {
    removePendingSlave(clientId);
    upsertSlave(clientId, ip);
    emitRegister(clientId, ip);
  }
}

void emitStaResult(const String& devId, const String& comp, int mod, int stat, int val) {
  DynamicJsonDocument doc(256);
  doc["devID"] = devId;
  doc["comp"] = comp;
  doc["mod"] = mod;
  doc["stat"] = stat;
  doc["val"] = val;
  logLine("WebSocket emit staresult dev=" + devId + " comp=" + comp);
  emitEvent("staresult", doc);
}

void emitResponse(const String& devId, const String& comp, int mod, int stat, int val) {
  DynamicJsonDocument doc(256);
  doc["devID"] = devId;
  doc["comp"] = comp;
  doc["mod"] = mod;
  doc["stat"] = stat;
  doc["val"] = val;
  logLine("WebSocket emit response dev=" + devId + " comp=" + comp);
  emitEvent("response", doc);
}

void handleTcpLine(const String& line, const String& remoteIp) {
  logLine("TCP <- " + remoteIp + " " + line);
  if (line.startsWith("drg=")) {
    String payload = line.substring(4);
    String tokens[2];
    int count = splitTokens(payload, ';', tokens, 2);
    if (count >= 2) {
      String clientId = normalizeId(tokens[0]);
      String ip = tokens[1];
      recordSeen(clientId, ip, true);
      bool canBind = !REQUIRE_BINDING || isPendingSlave(clientId) || isKnownSlave(clientId);
      if (!canBind) {
        logLine("Bind required, ignoring drg for " + clientId);
        return;
      }
      if (isPendingSlave(clientId)) {
        removePendingSlave(clientId);
      }
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
    if (REQUIRE_BINDING && !isKnownSlave(clientId)) {
      logLine("Ignoring update from unbound slave " + clientId);
      return;
    }
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
    logLine("Command skip: unknown slave " + devId);
    return;
  }
  String path = "/?usrcmd=" + devId + ";" + comp + ";" + String(mod) + ";" +
                String(stat) + ";" + String(val) + ";";
  sendHttpGet(ip, path);
}

void sendStatusToSlave(const String& devId, const String& comp) {
  String ip = findSlaveIp(devId);
  if (ip.length() == 0) {
    logLine("Status skip: unknown slave " + devId);
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
  logLine("WebSocket cmd dev=" + devId + " comp=" + comp);
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
    logLine("WebSocket status dev=" + devId + " comp=*" );
    sendStatusToSlave(devId, "Comp0");
    sendStatusToSlave(devId, "Comp1");
    sendStatusToSlave(devId, "Comp2");
  } else {
    logLine("WebSocket status dev=" + devId + " comp=" + comp);
    sendStatusToSlave(devId, comp);
  }
}

void handleBindSlave(JsonObject data) {
  String serverId = normalizeId(String(data["serverID"] | ""));
  if (serverId.length() > 0 && serverId != SERVER_ID) {
    return;
  }
  String clientId = normalizeId(String(data["clientID"] | ""));
  if (clientId.length() == 0) {
    return;
  }
  addPendingSlave(clientId);
  logLine("Bind requested for slave " + clientId);

  String ip = findSeenIp(clientId);
  if (ip.length() > 0) {
    removePendingSlave(clientId);
    upsertSlave(clientId, ip);
    emitRegister(clientId, ip);
  }
}

void handleUnbindSlave(JsonObject data) {
  String serverId = normalizeId(String(data["serverID"] | ""));
  if (serverId.length() > 0 && serverId != SERVER_ID) {
    return;
  }
  String clientId = normalizeId(String(data["clientID"] | ""));
  if (clientId.length() == 0) {
    return;
  }
  removePendingSlave(clientId);
  for (size_t i = 0; i < slaveCount; i++) {
    if (slaves[i].id == clientId) {
      slaves[i] = slaves[slaveCount - 1];
      slaveCount--;
      break;
    }
  }
  logLine("Unbound slave " + clientId);
}

void onWebSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  logLine(String("WebSocket type ") + webSocketTypeName(type));
  if (type == WStype_CONNECTED) {
    logLine("WebSocket connected");
    emitGatewayRegister();
    flushSeen();
    return;
  }
  if (type == WStype_DISCONNECTED) {
    logLine("WebSocket disconnected");
    return;
  }
  if (type == WStype_ERROR) {
    logLine("WebSocket error: " + payloadToString(payload, length));
    return;
  }
  if (type != WStype_TEXT) {
    return;
  }

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error) {
    logLine("WebSocket JSON parse error");
    return;
  }
  const char* eventName = doc["event"];
  JsonObject data = doc["data"].as<JsonObject>();
  if (!eventName || data.isNull()) {
    return;
  }

  logLine("WebSocket event <- " + String(eventName));

  if (strcmp(eventName, "command") == 0) {
    handleCommand(data);
  } else if (strcmp(eventName, "status") == 0) {
    handleStatus(data);
  } else if (strcmp(eventName, "bind_slave") == 0) {
    handleBindSlave(data);
  } else if (strcmp(eventName, "unbind_slave") == 0) {
    handleUnbindSlave(data);
  }
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    logLine("WiFi connecting...");
  }
  logLine("WiFi connected IP=" + WiFi.localIP().toString());

  tcpServer.begin();
  udp.begin(UDP_DISCOVERY_PORT);

  webSocket.beginSSL(SOCKET_HOST, SOCKET_PORT, SOCKET_PATH);
  webSocket.onEvent(onWebSocketEvent);
  webSocket.setReconnectInterval(5000);

  logLine("WebSocket WSS starting...");
}

void loop() {
  webSocket.loop();
  handleTcpClients();
  handleUdpDiscovery();
}
