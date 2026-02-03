#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// Wi-Fi credentials
const char* WIFI_SSID = "Nuron";
const char* WIFI_PASS = "mnbvcx12";

// Master TCP target
const char* MASTER_IP = "192.168.1.50";
const uint16_t MASTER_PORT = 6000;

// Slave identity (wire ID without RSW-)
const char* SLAVE_ID = "5678";

// Relay pins (active-LOW)
const uint8_t RELAY_PINS[4] = {D1, D2, D5, D6};

ESP8266WebServer server(80);
WiFiClient masterClient;

int relayState[4] = {0, 0, 0, 0};
unsigned long lastRegistrationMs = 0;
const unsigned long REGISTRATION_INTERVAL_MS = 60000;

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

void sendLine(const String& line) {
  if (!masterClient.connected()) {
    return;
  }
  masterClient.print(line + "\n");
}

void sendRegistration() {
  sendLine("drg=" + String(SLAVE_ID) + ";" + WiFi.localIP().toString());
  for (int i = 0; i < 4; i++) {
    String comp = "Comp" + String(i);
    String payload = String(SLAVE_ID) + ";" + comp + ";M;" + String(relayState[i]) + ";0";
    sendLine("dst=" + payload);
  }
}

void sendStatus(int index) {
  String comp = "Comp" + String(index);
  String payload = String(SLAVE_ID) + ";" + comp + ";M;" + String(relayState[index]) + ";0";
  sendLine("sta=" + payload);
}

void handleCommand() {
  if (!server.hasArg("usrcmd")) {
    server.send(400, "text/plain", "missing");
    return;
  }

  String cmd = server.arg("usrcmd");
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
  applyRelayState(index, stat);
  sendStatus(index);

  server.send(200, "text/plain", "ok");
}

void handleStatusRequest() {
  if (!server.hasArg("usrini")) {
    server.send(400, "text/plain", "missing");
    return;
  }

  String cmd = server.arg("usrini");
  cmd.trim();

  String parts[2];
  int count = 0;
  int start = 0;
  for (int i = 0; i < cmd.length() && count < 2; i++) {
    if (cmd[i] == ';') {
      parts[count++] = cmd.substring(start, i);
      start = i + 1;
    }
  }
  if (count < 2) {
    server.send(400, "text/plain", "invalid");
    return;
  }

  if (parts[0] != String(SLAVE_ID)) {
    server.send(404, "text/plain", "not_for_me");
    return;
  }

  int index = compToIndex(parts[1]);
  if (index < 0) {
    server.send(400, "text/plain", "invalid_comp");
    return;
  }

  sendStatus(index);
  server.send(200, "text/plain", "ok");
}

void ensureMasterConnection() {
  if (masterClient.connected()) {
    unsigned long now = millis();
    if (now - lastRegistrationMs >= REGISTRATION_INTERVAL_MS) {
      sendRegistration();
      lastRegistrationMs = now;
    }
    return;
  }
  masterClient.stop();
  if (masterClient.connect(MASTER_IP, MASTER_PORT)) {
    sendRegistration();
    lastRegistrationMs = millis();
  }
}

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < 4; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    applyRelayState(i, 0);
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

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

  server.begin();
  ensureMasterConnection();
}

void loop() {
  server.handleClient();
  ensureMasterConnection();
}
