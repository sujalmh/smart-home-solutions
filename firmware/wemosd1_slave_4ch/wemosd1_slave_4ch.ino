#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>

// Wi-Fi credentials
const char* WIFI_SSID = "Nuron";
const char* WIFI_PASS = "mnbvcx12";

// Master TCP target
const char* MASTER_HOST = "master-gateway.local";
const uint16_t MASTER_PORT = 6000;
const uint16_t UDP_DISCOVERY_PORT = 6000;
const unsigned long DISCOVERY_INTERVAL_MS = 12000;

// Slave identity (wire ID without RSW-)
const char* SLAVE_ID = "5678";

// Relay pins (active-LOW)
const uint8_t RELAY_PINS[4] = {D1, D2, D5, D6};

ESP8266WebServer server(80);
WiFiUDP udp;

int relayState[4] = {0, 0, 0, 0};
int relayMode[4] = {0, 0, 0, 0};
int relayValue[4] = {0, 0, 0, 0};
unsigned long lastRegistrationMs = 0;
const unsigned long REGISTRATION_INTERVAL_MS = 15000;
unsigned long lastDiscoveryMs = 0;
IPAddress masterIp;
unsigned long lastResolveMs = 0;
const unsigned long RESOLVE_INTERVAL_MS = 10000;
const unsigned long STATUS_SEND_INTERVAL_MS = 35;
bool pendingStatus[4] = {false, false, false, false};
unsigned long lastStatusSendMs = 0;
uint8_t statusCursor = 0;

int compToIndex(const String& comp) {
  if (comp == "Comp0") return 0;
  if (comp == "Comp1") return 1;
  if (comp == "Comp2") return 2;
  if (comp == "Comp3") return 3;
  return -1;
}

void applyRelayState(int index, int stat) {
  relayState[index] = stat ? 1 : 0;
  digitalWrite(RELAY_PINS[index], relayState[index] ? LOW : HIGH);
}

bool resolveMasterIp() {
  unsigned long now = millis();
  if (now - lastResolveMs < RESOLVE_INTERVAL_MS && masterIp != IPAddress(0, 0, 0, 0)) {
    return true;
  }
  lastResolveMs = now;
  IPAddress resolved;
  if (WiFi.hostByName(MASTER_HOST, resolved)) {
    masterIp = resolved;
    Serial.println("Resolved master IP: " + masterIp.toString());
    return true;
  }
  if (masterIp != IPAddress(0, 0, 0, 0)) {
    Serial.println("Host resolve failed; using discovered master IP: " + masterIp.toString());
    return true;
  }
  Serial.println("Failed to resolve master host.");
  return false;
}

void handleUdpAnnouncements() {
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
  if (!line.startsWith("mrg=")) {
    return;
  }

  String payload = line.substring(4);
  int sep = payload.indexOf(';');
  if (sep < 0) {
    return;
  }
  String ipStr = payload.substring(sep + 1);
  IPAddress announcedIp;
  if (!announcedIp.fromString(ipStr)) {
    return;
  }
  if (masterIp != announcedIp) {
    masterIp = announcedIp;
    Serial.println("Discovered master IP via UDP: " + masterIp.toString());
  }
}

bool sendLinesToMaster(const String* lines, size_t count) {
  if (!resolveMasterIp()) {
    return false;
  }
  WiFiClient client;
  Serial.println("Connecting to master TCP...");
  if (!client.connect(masterIp, MASTER_PORT)) {
    Serial.println("Master TCP connect failed.");
    return false;
  }
  Serial.println("Master TCP connected.");
  for (size_t i = 0; i < count; i++) {
    Serial.println("TCP -> " + lines[i]);
    client.print(lines[i] + "\n");
  }
  client.flush();
  client.stop();
  Serial.println("Master TCP closed.");
  return true;
}

void sendUdpDiscovery() {
  String line = "drg=" + String(SLAVE_ID) + ";" + WiFi.localIP().toString();
  udp.beginPacket(IPAddress(255, 255, 255, 255), UDP_DISCOVERY_PORT);
  udp.write(reinterpret_cast<const uint8_t*>(line.c_str()), line.length());
  udp.endPacket();
  Serial.println("UDP -> " + line);
}

void sendRegistration() {
  Serial.println("Registering with master...");
  String lines[5];
  lines[0] = "drg=" + String(SLAVE_ID) + ";" + WiFi.localIP().toString();
  for (int i = 0; i < 4; i++) {
    String comp = "Comp" + String(i);
    String payload = String(SLAVE_ID) + ";" + comp + ";" + String(relayMode[i]) + ";" +
                     String(relayState[i]) + ";" + String(relayValue[i]);
    lines[i + 1] = "dst=" + payload;
  }
  if (sendLinesToMaster(lines, 5)) {
    lastRegistrationMs = millis();
  }
}

bool sendStatus(int index) {
  String comp = "Comp" + String(index);
  String payload = String(SLAVE_ID) + ";" + comp + ";" + String(relayMode[index]) + ";" +
                   String(relayState[index]) + ";" + String(relayValue[index]);
  String line = "sta=" + payload;
  return sendLinesToMaster(&line, 1);
}

void enqueueStatus(int index) {
  if (index < 0 || index >= 4) {
    return;
  }
  pendingStatus[index] = true;
}

void processPendingStatus() {
  unsigned long now = millis();
  if (now - lastStatusSendMs < STATUS_SEND_INTERVAL_MS) {
    return;
  }

  for (int step = 0; step < 4; step++) {
    int index = (statusCursor + step) % 4;
    if (!pendingStatus[index]) {
      continue;
    }

    if (sendStatus(index)) {
      pendingStatus[index] = false;
    }
    statusCursor = static_cast<uint8_t>((index + 1) % 4);
    lastStatusSendMs = now;
    return;
  }
}

void handleCommand() {
  if (!server.hasArg("usrcmd")) {
    server.send(400, "text/plain", "missing");
    return;
  }

  String cmd = server.arg("usrcmd");
  Serial.println("HTTP usrcmd: " + cmd);
  cmd.trim();
  if (!cmd.endsWith(";")) {
    cmd += ";";
  }

  String parts[5];
  int count = 0;
  int start = 0;
  for (int i = 0; i < cmd.length() && count < 5; i++) {
    if (cmd[i] == ';') {
      parts[count++] = cmd.substring(start, i);
      start = i + 1;
    }
  }
  if (count < 4) {
    server.send(400, "text/plain", "invalid");
    return;
  }

  String slave = parts[0];
  String comp = parts[1];
  String mode = parts[2];
  String statStr = parts[3];
  String valStr = count >= 5 ? parts[4] : "";

  if (slave != String(SLAVE_ID)) {
    server.send(404, "text/plain", "not_for_me");
    return;
  }

  int index = compToIndex(comp);
  if (index < 0) {
    server.send(400, "text/plain", "invalid_comp");
    return;
  }

  int stat = statStr.toInt();
  int modeInt = mode.toInt();
  int value = valStr.toInt();
  stat = stat > 0 ? 1 : 0;
  modeInt = modeInt > 0 ? 1 : 0;
  if (value < 0) {
    value = 0;
  }
  if (value > 1000) {
    value = 1000;
  }

  relayMode[index] = modeInt;
  relayValue[index] = value;
  applyRelayState(index, stat);
  server.send(200, "text/plain", "ok");
  enqueueStatus(index);
}

void handleStatusRequest() {
  if (!server.hasArg("usrini")) {
    server.send(400, "text/plain", "missing");
    return;
  }

  String cmd = server.arg("usrini");
  Serial.println("HTTP usrini: " + cmd);
  cmd.trim();

  int sep = cmd.indexOf(';');
  if (sep < 0) {
    server.send(400, "text/plain", "invalid");
    return;
  }

  String slaveId = cmd.substring(0, sep);
  String comp = cmd.substring(sep + 1);
  int trailing = comp.indexOf(';');
  if (trailing >= 0) {
    comp = comp.substring(0, trailing);
  }
  slaveId.trim();
  comp.trim();
  if (slaveId.length() == 0 || comp.length() == 0) {
    server.send(400, "text/plain", "invalid");
    return;
  }

  if (slaveId != String(SLAVE_ID)) {
    server.send(404, "text/plain", "not_for_me");
    return;
  }

  int index = compToIndex(comp);
  if (index < 0) {
    server.send(400, "text/plain", "invalid_comp");
    return;
  }

  server.send(200, "text/plain", "ok");
  enqueueStatus(index);
}

void ensureRegistration() {
  unsigned long now = millis();
  if (now - lastRegistrationMs >= REGISTRATION_INTERVAL_MS) {
    Serial.println("Re-registering with master...");
    sendRegistration();
  }
}

void ensureDiscovery() {
  unsigned long now = millis();
  if (now - lastDiscoveryMs >= DISCOVERY_INTERVAL_MS) {
    sendUdpDiscovery();
    lastDiscoveryMs = now;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Slave booting...");

  for (int i = 0; i < 4; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    applyRelayState(i, 0);
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("WiFi connecting...");
  }
  Serial.println("WiFi connected IP=" + WiFi.localIP().toString());

  udp.begin(UDP_DISCOVERY_PORT);
  sendUdpDiscovery();
  lastDiscoveryMs = millis();

  server.on("/", HTTP_GET, []() {
    if (server.hasArg("usrcmd")) {
      handleCommand();
      return;
    }
    if (server.hasArg("usrini")) {
      handleStatusRequest();
      return;
    }
    server.send(200, "text/plain", "ok");
  });

  server.onNotFound([]() {
    Serial.println("HTTP 404: " + server.uri());
    server.send(404, "text/plain", "not_found");
  });

  server.begin();
  sendRegistration();
}

void loop() {
  server.handleClient();
  handleUdpAnnouncements();
  processPendingStatus();
  ensureRegistration();
  ensureDiscovery();
}
