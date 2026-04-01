#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include "mcp4728.h"
#include "Adafruit_MCP23017.h"

// Fixed legacy layout from RoomSwicthControl-old-reference.ino
// Comp0 -> SW1/FAN       (A7 + DAC1, input B1, LED A2)
// Comp1 -> SW0/Dim Light (A6 + DAC0, input B0, LED A3)
// Comp2 -> SW2/On/Off    (A0, no DAC, input B2, LED A1)

#define NUM_CHANNELS 3

#ifndef SLAVE_ID
#define SLAVE_ID "9002"
#endif

#ifndef WIFI_SSID_CFG
#define WIFI_SSID_CFG "Nuron"
#endif

#ifndef WIFI_PASS_CFG
#define WIFI_PASS_CFG "mnbvcx12"
#endif

// MCP23017 pin aliases (legacy reference)
#define M_A0 0
#define M_A1 1
#define M_A2 2
#define M_A3 3
#define M_A4 4
#define M_A5 5
#define M_A6 6
#define M_A7 7

#define M_B0 8
#define M_B1 9
#define M_B2 10
#define M_B3 11
#define M_B4 12
#define M_B5 13
#define M_B6 14
#define M_B7 15

static const char *MASTER_HOST = "master-gateway.local";
static const uint16_t MASTER_PORT = 6000;
static const uint16_t UDP_DISCOVERY_PORT = 6000;

static const unsigned long DISCOVERY_BASE_MS = 2000;
static const unsigned long DISCOVERY_MAX_MS = 60000;
static const unsigned long DISCOVERY_ACTIVE_MS = 45000;
static const unsigned long REGISTRATION_INTERVAL_MS = 6000;
static const unsigned long MASTER_STALE_MS = 30000;
static const unsigned long RESOLVE_INTERVAL_MS = 10000;
static const unsigned long STATUS_SEND_INTERVAL_MS = 35;
static const unsigned long WIFI_RECONNECT_INTERVAL_MS = 5000;
static const unsigned long TCP_CONNECT_TIMEOUT_MS = 800;
static const unsigned long LOOP_WDT_WARN_MS = 2000;

static const uint8_t WIFI_MAX_FAILURES = 5;
static const uint8_t STALE_DISC_FAILURES_RESET = 3;
static const uint8_t BACKOFF_JITTER_PCT = 25;
static const uint8_t MAX_CONSEC_DISC_FAILURES = 10;

struct ChannelHw {
  uint8_t inputPin;
  uint8_t ctrlPin;
  uint8_t ledPin;
  bool dimmable;
  uint8_t dacChannel;
  uint16_t minMv;
  uint16_t maxMv;
};

// Exact legacy component mapping
static const ChannelHw CHANNEL_HW[NUM_CHANNELS] = {
    {M_B1, M_A7, M_A2, true, 1, 50, 2500},
    {M_B0, M_A6, M_A3, true, 0, 10, 3050},
    {M_B2, M_A0, M_A1, false, 0, 0, 0},
};

enum SlaveState : uint8_t {
  STATE_DISCONNECTED,
  STATE_DISCOVERING,
  STATE_REGISTERING,
  STATE_LINKED,
  STATE_STALE
};

static const char *stateNames[] = {
    "DISCONNECTED", "DISCOVERING", "REGISTERING", "LINKED", "STALE"};

ESP8266WebServer httpServer(80);
WiFiUDP udp;

Adafruit_MCP23017 mcp;
mcp4728 dac = mcp4728(0);

int relayState[NUM_CHANNELS];
int relayMode[NUM_CHANNELS];
int relayValue[NUM_CHANNELS];
String pendingReqId[NUM_CHANNELS];
bool pendingStatus[NUM_CHANNELS];
uint8_t statusCursor = 0;
unsigned long lastStatusSendMs = 0;

int switchPos[NUM_CHANNELS];
bool switchToggled[NUM_CHANNELS];
int lastAnalogLevel = 0;

SlaveState currentState = STATE_DISCONNECTED;
IPAddress masterIp;
bool masterLinked = false;
unsigned long lastMasterContactMs = 0;
unsigned long lastRegistrationMs = 0;
unsigned long lastDiscoveryMs = 0;
unsigned long lastResolveMs = 0;

unsigned long discoveryIntervalMs = DISCOVERY_BASE_MS;
uint8_t consecDiscFailures = 0;
uint8_t staleDiscFailCount = 0;

unsigned long lastWifiCheckMs = 0;
uint8_t wifiConsecFailures = 0;

unsigned long loopStartMs = 0;

void sendRegistration();
void enqueueStatus(int index);

void logPrefixed(const char *prefix, const String &msg) {
  Serial.print('[');
  Serial.print(prefix);
  Serial.print("] ");
  Serial.println(msg);
}

#define LOG_STATE(m) logPrefixed("STATE", m)
#define LOG_WIFI(m) logPrefixed("WIFI", m)
#define LOG_DISC(m) logPrefixed("DISC", m)
#define LOG_REG(m) logPrefixed("REG", m)
#define LOG_CMD(m) logPrefixed("CMD", m)
#define LOG_STA(m) logPrefixed("STA", m)
#define LOG_WDT(m) logPrefixed("WDT", m)

void transitionTo(SlaveState newState, const String &reason) {
  if (newState == currentState) {
    return;
  }
  String msg = String(stateNames[currentState]) + " -> " +
               String(stateNames[newState]) + " reason=" + reason;
  LOG_STATE(msg);
  currentState = newState;
}

unsigned long applyJitter(unsigned long base) {
  long jitter = (long)base * BACKOFF_JITTER_PCT / 100;
  long offset = random(-jitter, jitter + 1);
  long result = (long)base + offset;
  return result > 0 ? (unsigned long)result : 1;
}

void resetDiscoveryBackoff() {
  discoveryIntervalMs = DISCOVERY_BASE_MS;
  consecDiscFailures = 0;
  staleDiscFailCount = 0;
}

void advanceDiscoveryBackoff() {
  consecDiscFailures++;
  if (consecDiscFailures >= MAX_CONSEC_DISC_FAILURES) {
    discoveryIntervalMs = DISCOVERY_MAX_MS;
  } else {
    discoveryIntervalMs = min(discoveryIntervalMs * 2, DISCOVERY_MAX_MS);
  }
}

int readClampedAnalogLevel() {
  int level = analogRead(A0);
  if (level < 0) {
    level = 0;
  }
  if (level > 950) {
    level = 950;
  }
  return level;
}

uint16_t levelToMilliVolts(int index, int level) {
  if (index < 0 || index >= NUM_CHANNELS) {
    return 0;
  }
  if (!CHANNEL_HW[index].dimmable) {
    return 0;
  }
  int bounded = constrain(level, 1, 950);
  return (uint16_t)map(bounded, 1, 950, CHANNEL_HW[index].minMv,
                       CHANNEL_HW[index].maxMv);
}

void applyChannelOutput(int index) {
  if (index < 0 || index >= NUM_CHANNELS) {
    return;
  }

  const ChannelHw &cfg = CHANNEL_HW[index];
  if (cfg.dimmable) {
    if (relayState[index] == 1) {
      if (relayValue[index] <= 0) {
        relayValue[index] = readClampedAnalogLevel();
      }
      uint16_t vtg = levelToMilliVolts(index, relayValue[index]);
      dac.voutWrite(cfg.dacChannel, vtg);
      delay(5);
      mcp.digitalWrite(cfg.ctrlPin, HIGH);
      mcp.digitalWrite(cfg.ledPin, HIGH);
    } else {
      mcp.digitalWrite(cfg.ctrlPin, LOW);
      mcp.digitalWrite(cfg.ledPin, LOW);
      dac.voutWrite(cfg.dacChannel, 0);
    }
    return;
  }

  mcp.digitalWrite(cfg.ctrlPin, relayState[index] ? HIGH : LOW);
  mcp.digitalWrite(cfg.ledPin, relayState[index] ? HIGH : LOW);
  relayValue[index] = 1000;
}

void applyRelayState(int index, int stat) {
  if (index < 0 || index >= NUM_CHANNELS) {
    return;
  }
  relayState[index] = stat ? 1 : 0;
  applyChannelOutput(index);
}

int compToIndex(const String &comp) {
  if (comp.length() == 5 && comp.startsWith("Comp")) {
    int idx = comp.charAt(4) - '0';
    if (idx >= 0 && idx < NUM_CHANNELS) {
      return idx;
    }
  }
  return -1;
}

void initializeMcp23017() {
  mcp.begin();

  mcp.pinMode(M_B0, INPUT);
  mcp.pullUp(M_B0, HIGH);
  mcp.pinMode(M_B1, INPUT);
  mcp.pullUp(M_B1, HIGH);
  mcp.pinMode(M_B2, INPUT);
  mcp.pullUp(M_B2, HIGH);

  mcp.pinMode(M_A0, OUTPUT);
  mcp.pinMode(M_A1, OUTPUT);
  mcp.pinMode(M_A2, OUTPUT);
  mcp.pinMode(M_A3, OUTPUT);
  mcp.pinMode(M_A6, OUTPUT);
  mcp.pinMode(M_A7, OUTPUT);

  mcp.digitalWrite(M_A0, LOW);
  mcp.digitalWrite(M_A1, HIGH);
  mcp.digitalWrite(M_A2, HIGH);
  mcp.digitalWrite(M_A3, HIGH);
  mcp.digitalWrite(M_A6, LOW);
  mcp.digitalWrite(M_A7, LOW);

  delay(300);

  mcp.digitalWrite(M_A1, LOW);
  mcp.digitalWrite(M_A2, LOW);
  mcp.digitalWrite(M_A3, LOW);
}

void initializeMcp4728() {
  dac.begin();
  dac.vdd(3300);
  dac.setVref(0, 0);
  dac.setVref(1, 0);
  dac.setGain(0, 0, 0, 0);
  dac.setPowerDown(0, 0, 0, 0);
  dac.voutWrite(0, 0);
  dac.voutWrite(1, 0);
}

void initializeChannelState() {
  lastAnalogLevel = readClampedAnalogLevel();

  for (int i = 0; i < NUM_CHANNELS; i++) {
    relayMode[i] = 1;
    pendingReqId[i] = "";
    pendingStatus[i] = false;
    switchToggled[i] = false;

    switchPos[i] = mcp.digitalRead(CHANNEL_HW[i].inputPin);
    relayState[i] = (switchPos[i] == LOW) ? 1 : 0;
    relayValue[i] = CHANNEL_HW[i].dimmable ? lastAnalogLevel : 1000;
    applyChannelOutput(i);
  }
}

void readLocalSwitches() {
  for (int i = 0; i < NUM_CHANNELS; i++) {
    int newPos = mcp.digitalRead(CHANNEL_HW[i].inputPin);
    if (newPos != switchPos[i]) {
      switchPos[i] = newPos;
      switchToggled[i] = true;
    }
  }

  int newAnalogLevel = readClampedAnalogLevel();
  if (abs(newAnalogLevel - lastAnalogLevel) > 4) {
    lastAnalogLevel = newAnalogLevel;
    for (int i = 0; i < NUM_CHANNELS; i++) {
      if (CHANNEL_HW[i].dimmable && relayState[i] == 1) {
        relayValue[i] = newAnalogLevel;
        applyChannelOutput(i);
        enqueueStatus(i);
      }
    }
  }
}

void handleLocalSwitches() {
  for (int i = 0; i < NUM_CHANNELS; i++) {
    if (!switchToggled[i]) {
      continue;
    }
    switchToggled[i] = false;
    relayState[i] = relayState[i] ? 0 : 1;
    if (CHANNEL_HW[i].dimmable && relayState[i] == 1 && relayValue[i] <= 0) {
      relayValue[i] = lastAnalogLevel;
    }
    applyChannelOutput(i);
    LOG_CMD("Local toggle Comp" + String(i) + " -> " + String(relayState[i]));
    enqueueStatus(i);
  }
}

bool resolveMasterIp() {
  unsigned long now = millis();
  if (now - lastResolveMs < RESOLVE_INTERVAL_MS &&
      masterIp != IPAddress(0, 0, 0, 0)) {
    return true;
  }

  lastResolveMs = now;
  IPAddress resolved;
  if (WiFi.hostByName(MASTER_HOST, resolved)) {
    masterIp = resolved;
    LOG_DISC("Resolved master IP: " + masterIp.toString());
    return true;
  }
  if (masterIp != IPAddress(0, 0, 0, 0)) {
    LOG_DISC("Resolve failed; using cached master IP: " + masterIp.toString());
    return true;
  }
  LOG_DISC("Failed to resolve master host");
  return false;
}

bool sendLinesToMaster(const String *lines, size_t count) {
  if (!resolveMasterIp()) {
    if (millis() - lastMasterContactMs > MASTER_STALE_MS) {
      transitionTo(STATE_STALE, "resolve_fail");
    }
    advanceDiscoveryBackoff();
    return false;
  }

  WiFiClient client;
  client.setTimeout(TCP_CONNECT_TIMEOUT_MS);
  LOG_REG("TCP connecting to " + masterIp.toString() + ":" + String(MASTER_PORT));

  if (!client.connect(masterIp, MASTER_PORT)) {
    LOG_REG("TCP connect failed");
    if (millis() - lastMasterContactMs > MASTER_STALE_MS) {
      transitionTo(STATE_STALE, "tcp_connect_fail");
    }
    advanceDiscoveryBackoff();
    return false;
  }

  for (size_t i = 0; i < count; i++) {
    LOG_STA("TCP -> " + lines[i]);
    client.print(lines[i] + "\n");
  }
  client.flush();
  client.stop();

  masterLinked = true;
  lastMasterContactMs = millis();
  resetDiscoveryBackoff();
  LOG_REG("TCP send OK, " + String(count) + " lines");
  return true;
}

void sendUdpDiscovery() {
  String line = "drg=" + String(SLAVE_ID) + ";" + WiFi.localIP().toString();
  udp.beginPacket(IPAddress(255, 255, 255, 255), UDP_DISCOVERY_PORT);
  udp.write(reinterpret_cast<const uint8_t *>(line.c_str()), line.length());
  udp.endPacket();
  LOG_DISC("UDP -> " + line);
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
    masterLinked = false;
    LOG_DISC("Discovered master IP via UDP: " + masterIp.toString());
  }

  resetDiscoveryBackoff();

  if (currentState == STATE_DISCONNECTED || currentState == STATE_DISCOVERING ||
      currentState == STATE_STALE) {
    transitionTo(STATE_REGISTERING, "mrg_received");
    sendRegistration();
  }
}

void sendRegistration() {
  LOG_REG("Registering with master (channels=" + String(NUM_CHANNELS) + ")");
  String lines[1 + NUM_CHANNELS];

  lines[0] = "drg=" + String(SLAVE_ID) + ";" + WiFi.localIP().toString();
  for (int i = 0; i < NUM_CHANNELS; i++) {
    String comp = "Comp" + String(i);
    String payload = String(SLAVE_ID) + ";" + comp + ";" + String(relayMode[i]) +
                     ";" + String(relayState[i]) + ";" + String(relayValue[i]);
    lines[i + 1] = "dst=" + payload;
  }

  if (sendLinesToMaster(lines, 1 + NUM_CHANNELS)) {
    lastRegistrationMs = millis();
    if (currentState == STATE_REGISTERING || currentState == STATE_STALE) {
      transitionTo(STATE_LINKED, "registration_ok");
    }
  }
}

bool sendStatus(int index) {
  if (index < 0 || index >= NUM_CHANNELS) {
    return false;
  }

  String comp = "Comp" + String(index);
  String payload = String(SLAVE_ID) + ";" + comp + ";" + String(relayMode[index]) +
                   ";" + String(relayState[index]) + ";" +
                   String(relayValue[index]);

  if (pendingReqId[index].length() > 0) {
    payload += ";" + pendingReqId[index];
  }

  String line = "sta=" + payload;
  bool sent = sendLinesToMaster(&line, 1);
  if (sent && pendingReqId[index].length() > 0) {
    pendingReqId[index] = "";
  }
  return sent;
}

void enqueueStatus(int index) {
  if (index < 0 || index >= NUM_CHANNELS) {
    return;
  }
  pendingStatus[index] = true;
}

void processPendingStatus() {
  unsigned long now = millis();
  if (now - lastStatusSendMs < STATUS_SEND_INTERVAL_MS) {
    return;
  }

  for (int step = 0; step < NUM_CHANNELS; step++) {
    int index = (statusCursor + step) % NUM_CHANNELS;
    if (!pendingStatus[index]) {
      continue;
    }

    if (sendStatus(index)) {
      pendingStatus[index] = false;
    }
    statusCursor = static_cast<uint8_t>((index + 1) % NUM_CHANNELS);
    lastStatusSendMs = now;
    return;
  }
}

void handleCommand() {
  if (!httpServer.hasArg("usrcmd")) {
    httpServer.send(400, "text/plain", "missing");
    return;
  }

  String cmd = httpServer.arg("usrcmd");
  LOG_CMD("HTTP usrcmd: " + cmd);
  cmd.trim();
  if (!cmd.endsWith(";")) {
    cmd += ";";
  }

  String parts[6];
  int count = 0;
  int start = 0;
  for (unsigned int i = 0; i < cmd.length() && count < 6; i++) {
    if (cmd[i] == ';') {
      parts[count++] = cmd.substring(start, i);
      start = i + 1;
    }
  }

  if (count < 4) {
    httpServer.send(400, "text/plain", "invalid");
    return;
  }

  if (parts[0] != String(SLAVE_ID)) {
    httpServer.send(404, "text/plain", "not_for_me");
    return;
  }

  int index = compToIndex(parts[1]);
  if (index < 0) {
    httpServer.send(400, "text/plain", "invalid_comp");
    return;
  }

  int modeInt = parts[2].toInt() > 0 ? 1 : 0;
  int stat = parts[3].toInt() > 0 ? 1 : 0;
  int value = (count >= 5) ? parts[4].toInt() : 0;
  String reqId = (count >= 6) ? parts[5] : "";

  if (value < 0) {
    value = 0;
  }
  if (value > 1000) {
    value = 1000;
  }

  relayMode[index] = modeInt;
  relayValue[index] = CHANNEL_HW[index].dimmable ? value : 1000;
  pendingReqId[index] = reqId;
  applyRelayState(index, stat);

  LOG_CMD("Applied Comp" + String(index) + " stat=" + String(stat) +
          " mod=" + String(modeInt) + " val=" + String(relayValue[index]));

  httpServer.send(200, "text/plain", "ok");
  enqueueStatus(index);
}

void handleStatusRequest() {
  if (!httpServer.hasArg("usrini")) {
    httpServer.send(400, "text/plain", "missing");
    return;
  }

  String cmd = httpServer.arg("usrini");
  LOG_STA("HTTP usrini: " + cmd);
  cmd.trim();

  int sep = cmd.indexOf(';');
  if (sep < 0) {
    httpServer.send(400, "text/plain", "invalid");
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
    httpServer.send(400, "text/plain", "invalid");
    return;
  }

  if (slaveId != String(SLAVE_ID)) {
    httpServer.send(404, "text/plain", "not_for_me");
    return;
  }

  int index = compToIndex(comp);
  if (index < 0) {
    httpServer.send(400, "text/plain", "invalid_comp");
    return;
  }

  httpServer.send(200, "text/plain", "ok");
  enqueueStatus(index);
}

void handlePing() {
  httpServer.send(200, "text/plain", "pong");
}

void ensureRegistration() {
  unsigned long now = millis();
  if (now - lastRegistrationMs < REGISTRATION_INTERVAL_MS) {
    return;
  }
  if (currentState == STATE_LINKED &&
      (now - lastMasterContactMs < MASTER_STALE_MS)) {
    return;
  }

  LOG_REG("Re-registering with master...");
  sendRegistration();
}

void ensureDiscovery() {
  unsigned long now = millis();

  if (currentState == STATE_LINKED) {
    if (now - lastDiscoveryMs >= DISCOVERY_ACTIVE_MS) {
      sendUdpDiscovery();
      lastDiscoveryMs = now;
    }
    return;
  }

  unsigned long interval = applyJitter(discoveryIntervalMs);
  if (now - lastDiscoveryMs >= interval) {
    sendUdpDiscovery();
    lastDiscoveryMs = now;

    if (currentState == STATE_STALE) {
      staleDiscFailCount++;
      if (staleDiscFailCount >= STALE_DISC_FAILURES_RESET) {
        transitionTo(STATE_DISCONNECTED, "stale_disc_exhausted");
        staleDiscFailCount = 0;
      }
    }
  }
}

void checkMasterStale() {
  if (currentState != STATE_LINKED) {
    return;
  }
  unsigned long now = millis();
  if (now - lastMasterContactMs > MASTER_STALE_MS) {
    transitionTo(STATE_STALE, "no_contact_" + String(MASTER_STALE_MS) + "ms");
  }
}

void checkWifi() {
  if (WiFi.status() == WL_CONNECTED) {
    if (wifiConsecFailures > 0) {
      LOG_WIFI("Reconnected, IP=" + WiFi.localIP().toString());
      wifiConsecFailures = 0;
      transitionTo(STATE_DISCONNECTED, "wifi_reconnected");
      resetDiscoveryBackoff();
    }
    return;
  }

  unsigned long now = millis();
  if (now - lastWifiCheckMs < WIFI_RECONNECT_INTERVAL_MS) {
    return;
  }
  lastWifiCheckMs = now;

  wifiConsecFailures++;
  LOG_WIFI("Disconnected, attempt " + String(wifiConsecFailures) + "/" +
           String(WIFI_MAX_FAILURES));

  if (wifiConsecFailures >= WIFI_MAX_FAILURES) {
    LOG_WIFI("Max failures reached - restarting chip");
    delay(100);
    ESP.restart();
  }

  transitionTo(STATE_DISCONNECTED, "wifi_lost");
  WiFi.reconnect();
}

void checkLoopWatchdog() {
  unsigned long elapsed = millis() - loopStartMs;
  if (elapsed > LOOP_WDT_WARN_MS) {
    LOG_WDT("loop iteration " + String(elapsed) + "ms (>" +
            String(LOOP_WDT_WARN_MS) + "ms)");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("=======================================");
  Serial.println(" Wemos D1 Legacy-Mapped Slave");
  Serial.println(" ID:       " + String(SLAVE_ID));
  Serial.println(" Channels: " + String(NUM_CHANNELS));
  Serial.println("=======================================");

  randomSeed(analogRead(A0) ^ micros());

  Wire.begin();
  initializeMcp23017();
  initializeMcp4728();
  initializeChannelState();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID_CFG, WIFI_PASS_CFG);
  LOG_WIFI("Connecting to " + String(WIFI_SSID_CFG));

  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    ESP.wdtFeed();
    if (millis() - wifiStart > 30000) {
      LOG_WIFI("Initial connect timeout - restarting");
      delay(100);
      ESP.restart();
    }
  }
  LOG_WIFI("Connected IP=" + WiFi.localIP().toString());

  udp.begin(UDP_DISCOVERY_PORT);
  sendUdpDiscovery();
  lastDiscoveryMs = millis();

  httpServer.on("/", HTTP_GET, []() {
    if (httpServer.hasArg("usrcmd")) {
      handleCommand();
      return;
    }
    if (httpServer.hasArg("usrini")) {
      handleStatusRequest();
      return;
    }
    if (httpServer.hasArg("ping")) {
      handlePing();
      return;
    }
    httpServer.send(200, "text/plain", "ok");
  });
  httpServer.onNotFound([]() {
    Serial.println("HTTP 404: " + httpServer.uri());
    httpServer.send(404, "text/plain", "not_found");
  });
  httpServer.begin();

  transitionTo(STATE_DISCOVERING, "boot");
  sendRegistration();

  LOG_STATE("Setup complete, entering loop");
}

void loop() {
  loopStartMs = millis();
  ESP.wdtFeed();

  checkWifi();
  if (WiFi.status() != WL_CONNECTED) {
    delay(10);
    checkLoopWatchdog();
    return;
  }

  httpServer.handleClient();
  handleUdpAnnouncements();

  readLocalSwitches();
  handleLocalSwitches();

  processPendingStatus();
  ensureRegistration();
  ensureDiscovery();
  checkMasterStale();
  checkLoopWatchdog();
}
