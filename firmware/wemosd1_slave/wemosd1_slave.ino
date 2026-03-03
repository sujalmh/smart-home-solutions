// ═══════════════════════════════════════════════════════════════════════════════
// Wemos D1 Relay Slave — Unified Parameterised Firmware
// ═══════════════════════════════════════════════════════════════════════════════
//
// Supports 1-channel and 4-channel relay boards via NUM_CHANNELS.
// Set NUM_CHANNELS, SLAVE_ID, RELAY_PINS[], WIFI_SSID, WIFI_PASS at compile.
//
// ┌────────────────────────────────────────────────────────────────────────────┐
// │ STATE MACHINE                                                             │
// │                                                                           │
// │   ┌──────────────┐   WiFi OK    ┌──────────────┐  mrg= received          │
// │   │ DISCONNECTED │─────────────►│ DISCOVERING  │──────────────┐           │
// │   │              │◄─────────────│              │              │           │
// │   └──────┬───────┘  3× fail     └──────────────┘              ▼           │
// │          │                                              ┌────────────┐    │
// │          │  WiFi lost                                   │ REGISTERING│    │
// │          │  at any state                                │            │    │
// │          │                                              └──────┬─────┘    │
// │          │                                                     │          │
// │          │                ┌─────────┐   no TCP >30s     ┌──────▼─────┐    │
// │          │                │  STALE  │◄──────────────────│  LINKED    │    │
// │          │                │         │──────────────────►│            │    │
// │          │                └────┬────┘  re-registered    └────────────┘    │
// │          │                     │ 3× disc fail                             │
// │          │                     ▼                                           │
// │          │  ┌──────────────┐                                              │
// │          └──│ DISCONNECTED │                                              │
// │             └──────────────┘                                              │
// └────────────────────────────────────────────────────────────────────────────┘
//
// ┌────────────────────────────────────────────────────────────────────────────┐
// │ TIMING CONSTANTS                                                          │
// ├────────────────────────────────┬────────┬──────────────────────────────────┤
// │ Name                           │ Value  │ Purpose                          │
// ├────────────────────────────────┼────────┼──────────────────────────────────┤
// │ DISCOVERY_BASE_MS              │ 2000   │ Initial backoff base for disc.   │
// │ DISCOVERY_MAX_MS               │ 60000  │ Max backoff cap for discovery    │
// │ DISCOVERY_ACTIVE_MS            │ 45000  │ Keepalive disc. when LINKED      │
// │ REGISTRATION_INTERVAL_MS       │ 6000   │ Re-registration period           │
// │ MASTER_STALE_MS                │ 30000  │ No-contact → STALE transition    │
// │ RESOLVE_INTERVAL_MS            │ 10000  │ mDNS re-resolve interval         │
// │ STATUS_SEND_INTERVAL_MS        │ 35     │ Min spacing between TCP sends    │
// │ WIFI_RECONNECT_INTERVAL_MS     │ 5000   │ WiFi retry period                │
// │ WIFI_MAX_FAILURES              │ 5      │ Chip restart after N WiFi fails  │
// │ TCP_CONNECT_TIMEOUT_MS         │ 800    │ TCP connect timeout to master    │
// │ LOOP_WDT_WARN_MS               │ 2000   │ Warn if loop() exceeds this     │
// │ STALE_DISC_FAILURES_RESET      │ 3      │ Disc. failures → DISCONNECTED   │
// │ BACKOFF_JITTER_PCT             │ 25     │ ±% jitter on backoff intervals   │
// │ MAX_CONSEC_DISC_FAILURES       │ 10     │ After N, lock to max backoff     │
// │ PING_PORT                      │ 80     │ HTTP ping endpoint port          │
// └────────────────────────────────┴────────┴──────────────────────────────────┘
//
// ═══════════════════════════════════════════════════════════════════════════════

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>

// ─── BUILD CONFIGURATION ─────────────────────────────────────────────────────
// Change these per-variant at compile time.

#ifndef NUM_CHANNELS
#define NUM_CHANNELS 1             // 1 or 4
#endif

#ifndef SLAVE_ID
#define SLAVE_ID "9003"             // Wire ID without RSW-
#endif

#ifndef WIFI_SSID_CFG
#define WIFI_SSID_CFG "Nuron"
#endif

#ifndef WIFI_PASS_CFG
#define WIFI_PASS_CFG "mnbvcx12"
#endif

// Relay pin mapping — must match NUM_CHANNELS
#if NUM_CHANNELS == 4
static const uint8_t RELAY_PINS[NUM_CHANNELS] = {D1, D2, D5, D6};
#elif NUM_CHANNELS == 1
static const uint8_t RELAY_PINS[NUM_CHANNELS] = {D1};
#else
#error "NUM_CHANNELS must be 1 or 4"
#endif

// ─── NETWORK CONSTANTS ──────────────────────────────────────────────────────
static const char*    MASTER_HOST           = "master-gateway.local";
static const uint16_t MASTER_PORT           = 6000;
static const uint16_t UDP_DISCOVERY_PORT    = 6000;

// ─── TIMING CONSTANTS ───────────────────────────────────────────────────────
static const unsigned long DISCOVERY_BASE_MS           = 2000;
static const unsigned long DISCOVERY_MAX_MS            = 60000;
static const unsigned long DISCOVERY_ACTIVE_MS         = 45000;
static const unsigned long REGISTRATION_INTERVAL_MS    = 6000;
static const unsigned long MASTER_STALE_MS             = 30000;
static const unsigned long RESOLVE_INTERVAL_MS         = 10000;
static const unsigned long STATUS_SEND_INTERVAL_MS     = 35;
static const unsigned long WIFI_RECONNECT_INTERVAL_MS  = 5000;
static const unsigned long TCP_CONNECT_TIMEOUT_MS      = 800;
static const unsigned long LOOP_WDT_WARN_MS            = 2000;

static const uint8_t  WIFI_MAX_FAILURES           = 5;
static const uint8_t  STALE_DISC_FAILURES_RESET   = 3;
static const uint8_t  BACKOFF_JITTER_PCT          = 25;
static const uint8_t  MAX_CONSEC_DISC_FAILURES    = 10;

// ─── STATE MACHINE ──────────────────────────────────────────────────────────
enum SlaveState : uint8_t {
  STATE_DISCONNECTED,
  STATE_DISCOVERING,
  STATE_REGISTERING,
  STATE_LINKED,
  STATE_STALE
};

static const char* stateNames[] = {
  "DISCONNECTED", "DISCOVERING", "REGISTERING", "LINKED", "STALE"
};

// ─── GLOBAL STATE ───────────────────────────────────────────────────────────
ESP8266WebServer httpServer(80);
WiFiUDP udp;

// Relay state arrays  (length = NUM_CHANNELS)
int      relayState[NUM_CHANNELS];
int      relayMode[NUM_CHANNELS];
int      relayValue[NUM_CHANNELS];
String   pendingReqId[NUM_CHANNELS];
bool     pendingStatus[NUM_CHANNELS];
uint8_t  statusCursor        = 0;
unsigned long lastStatusSendMs = 0;

// Connection / discovery state
SlaveState currentState           = STATE_DISCONNECTED;
IPAddress  masterIp;
bool       masterLinked           = false;
unsigned long lastMasterContactMs = 0;
unsigned long lastRegistrationMs  = 0;
unsigned long lastDiscoveryMs     = 0;
unsigned long lastResolveMs       = 0;

// Backoff state
unsigned long discoveryIntervalMs   = DISCOVERY_BASE_MS;
uint8_t       consecDiscFailures    = 0;
uint8_t       staleDiscFailCount    = 0;

// WiFi reconnect
unsigned long lastWifiCheckMs       = 0;
uint8_t       wifiConsecFailures    = 0;

// Loop watchdog
unsigned long loopStartMs          = 0;

// ─── LOGGING HELPERS ────────────────────────────────────────────────────────

void logPrefixed(const char* prefix, const String& msg) {
  Serial.print('[');
  Serial.print(prefix);
  Serial.print("] ");
  Serial.println(msg);
}

#define LOG_STATE(m) logPrefixed("STATE", m)
#define LOG_WIFI(m)  logPrefixed("WIFI", m)
#define LOG_DISC(m)  logPrefixed("DISC", m)
#define LOG_REG(m)   logPrefixed("REG", m)
#define LOG_CMD(m)   logPrefixed("CMD", m)
#define LOG_STA(m)   logPrefixed("STA", m)
#define LOG_WDT(m)   logPrefixed("WDT", m)

// ─── STATE TRANSITIONS ─────────────────────────────────────────────────────

void transitionTo(SlaveState newState, const String& reason) {
  if (newState == currentState) return;
  String msg = String(stateNames[currentState]) + " -> " +
               String(stateNames[newState]) + " reason=" + reason;
  LOG_STATE(msg);
  currentState = newState;
}

// ─── BACKOFF HELPERS ────────────────────────────────────────────────────────

unsigned long applyJitter(unsigned long base) {
  long jitter = (long)base * BACKOFF_JITTER_PCT / 100;
  long offset = random(-jitter, jitter + 1);
  long result = (long)base + offset;
  return result > 0 ? (unsigned long)result : 1;
}

void resetDiscoveryBackoff() {
  discoveryIntervalMs  = DISCOVERY_BASE_MS;
  consecDiscFailures   = 0;
  staleDiscFailCount   = 0;
}

void advanceDiscoveryBackoff() {
  consecDiscFailures++;
  if (consecDiscFailures >= MAX_CONSEC_DISC_FAILURES) {
    discoveryIntervalMs = DISCOVERY_MAX_MS;
  } else {
    discoveryIntervalMs = min(discoveryIntervalMs * 2, DISCOVERY_MAX_MS);
  }
}

// ─── RELAY HELPERS ──────────────────────────────────────────────────────────

void applyRelayState(int index, int stat) {
  if (index < 0 || index >= NUM_CHANNELS) return;
  relayState[index] = stat ? 1 : 0;
  // Active-LOW relay: LOW = ON, HIGH = OFF
  digitalWrite(RELAY_PINS[index], relayState[index] ? LOW : HIGH);
}

int compToIndex(const String& comp) {
  // "Comp0" → 0, "Comp1" → 1, etc.
  if (comp.length() == 5 && comp.startsWith("Comp")) {
    int idx = comp.charAt(4) - '0';
    if (idx >= 0 && idx < NUM_CHANNELS) return idx;
  }
  return -1;
}

// ─── MASTER IP RESOLUTION ───────────────────────────────────────────────────

bool resolveMasterIp() {
  unsigned long now = millis();
  if (now - lastResolveMs < RESOLVE_INTERVAL_MS && masterIp != IPAddress(0, 0, 0, 0)) {
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

// ─── TCP SEND ───────────────────────────────────────────────────────────────

bool sendLinesToMaster(const String* lines, size_t count) {
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

// ─── UDP DISCOVERY ──────────────────────────────────────────────────────────

void sendUdpDiscovery() {
  String line = "drg=" + String(SLAVE_ID) + ";" + WiFi.localIP().toString();
  udp.beginPacket(IPAddress(255, 255, 255, 255), UDP_DISCOVERY_PORT);
  udp.write(reinterpret_cast<const uint8_t*>(line.c_str()), line.length());
  udp.endPacket();
  LOG_DISC("UDP -> " + line);
}

void handleUdpAnnouncements() {
  int packetSize = udp.parsePacket();
  if (packetSize <= 0) return;

  char buffer[128];
  int len = udp.read(buffer, sizeof(buffer) - 1);
  if (len <= 0) return;
  buffer[len] = '\0';

  String line = String(buffer);
  line.trim();
  if (!line.startsWith("mrg=")) return;

  String payload = line.substring(4);
  int sep = payload.indexOf(';');
  if (sep < 0) return;

  String ipStr = payload.substring(sep + 1);
  IPAddress announcedIp;
  if (!announcedIp.fromString(ipStr)) return;

  if (masterIp != announcedIp) {
    masterIp = announcedIp;
    masterLinked = false;
    LOG_DISC("Discovered master IP via UDP: " + masterIp.toString());
  }

  resetDiscoveryBackoff();

  // Got a master reply — transition toward registration
  if (currentState == STATE_DISCONNECTED || currentState == STATE_DISCOVERING ||
      currentState == STATE_STALE) {
    transitionTo(STATE_REGISTERING, "mrg_received");
    sendRegistration();
  }
}

// ─── REGISTRATION ───────────────────────────────────────────────────────────

void sendRegistration() {
  LOG_REG("Registering with master (channels=" + String(NUM_CHANNELS) + ")");
  String lines[1 + NUM_CHANNELS];

  lines[0] = "drg=" + String(SLAVE_ID) + ";" + WiFi.localIP().toString();
  for (int i = 0; i < NUM_CHANNELS; i++) {
    String comp = "Comp" + String(i);
    String payload = String(SLAVE_ID) + ";" + comp + ";" +
                     String(relayMode[i]) + ";" +
                     String(relayState[i]) + ";" +
                     String(relayValue[i]);
    lines[i + 1] = "dst=" + payload;
  }

  if (sendLinesToMaster(lines, 1 + NUM_CHANNELS)) {
    lastRegistrationMs = millis();
    if (currentState == STATE_REGISTERING || currentState == STATE_STALE) {
      transitionTo(STATE_LINKED, "registration_ok");
    }
  }
}

// ─── STATUS SEND ────────────────────────────────────────────────────────────

bool sendStatus(int index) {
  if (index < 0 || index >= NUM_CHANNELS) return false;
  String comp = "Comp" + String(index);
  String payload = String(SLAVE_ID) + ";" + comp + ";" +
                   String(relayMode[index]) + ";" +
                   String(relayState[index]) + ";" +
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
  if (index < 0 || index >= NUM_CHANNELS) return;
  pendingStatus[index] = true;
}

void processPendingStatus() {
  unsigned long now = millis();
  if (now - lastStatusSendMs < STATUS_SEND_INTERVAL_MS) return;

  for (int step = 0; step < NUM_CHANNELS; step++) {
    int index = (statusCursor + step) % NUM_CHANNELS;
    if (!pendingStatus[index]) continue;

    if (sendStatus(index)) {
      pendingStatus[index] = false;
    }
    statusCursor = static_cast<uint8_t>((index + 1) % NUM_CHANNELS);
    lastStatusSendMs = now;
    return;  // one per tick
  }
}

// ─── HTTP COMMAND HANDLER ───────────────────────────────────────────────────

void handleCommand() {
  if (!httpServer.hasArg("usrcmd")) {
    httpServer.send(400, "text/plain", "missing");
    return;
  }

  String cmd = httpServer.arg("usrcmd");
  LOG_CMD("HTTP usrcmd: " + cmd);
  cmd.trim();
  if (!cmd.endsWith(";")) cmd += ";";

  String parts[6];
  int count = 0, start = 0;
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
  int stat    = parts[3].toInt() > 0 ? 1 : 0;
  int value   = (count >= 5) ? parts[4].toInt() : 0;
  if (value < 0)    value = 0;
  if (value > 1000) value = 1000;
  String reqId = (count >= 6) ? parts[5] : "";

  relayMode[index]     = modeInt;
  relayValue[index]    = value;
  pendingReqId[index]  = reqId;
  applyRelayState(index, stat);

  LOG_CMD("Applied ch" + String(index) + " stat=" + String(stat) +
          " mod=" + String(modeInt) + " val=" + String(value));

  httpServer.send(200, "text/plain", "ok");
  enqueueStatus(index);
}

// ─── HTTP STATUS REQUEST HANDLER ────────────────────────────────────────────

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
  String comp    = cmd.substring(sep + 1);
  int trailing   = comp.indexOf(';');
  if (trailing >= 0) comp = comp.substring(0, trailing);
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

// ─── HTTP PING HANDLER ─────────────────────────────────────────────────────

void handlePing() {
  httpServer.send(200, "text/plain", "pong");
}

// ─── PERIODIC: REGISTRATION ────────────────────────────────────────────────

void ensureRegistration() {
  unsigned long now = millis();
  if (now - lastRegistrationMs < REGISTRATION_INTERVAL_MS) return;
  if (currentState == STATE_LINKED &&
      (now - lastMasterContactMs < MASTER_STALE_MS)) return;

  LOG_REG("Re-registering with master...");
  sendRegistration();
}

// ─── PERIODIC: DISCOVERY ───────────────────────────────────────────────────

void ensureDiscovery() {
  unsigned long now = millis();

  // When linked, use the fixed active interval (no backoff needed)
  if (currentState == STATE_LINKED) {
    if (now - lastDiscoveryMs >= DISCOVERY_ACTIVE_MS) {
      sendUdpDiscovery();
      lastDiscoveryMs = now;
    }
    return;
  }

  // When not linked, use backoff interval with jitter
  unsigned long interval = applyJitter(discoveryIntervalMs);
  if (now - lastDiscoveryMs >= interval) {
    sendUdpDiscovery();
    lastDiscoveryMs = now;

    // Track failures for STALE → DISCONNECTED transition
    if (currentState == STATE_STALE) {
      staleDiscFailCount++;
      if (staleDiscFailCount >= STALE_DISC_FAILURES_RESET) {
        transitionTo(STATE_DISCONNECTED, "stale_disc_exhausted");
        staleDiscFailCount = 0;
      }
    }
  }
}

// ─── PERIODIC: STALE CHECK ─────────────────────────────────────────────────

void checkMasterStale() {
  if (currentState != STATE_LINKED) return;
  unsigned long now = millis();
  if (now - lastMasterContactMs > MASTER_STALE_MS) {
    transitionTo(STATE_STALE, "no_contact_" + String(MASTER_STALE_MS) + "ms");
  }
}

// ─── WIFI MONITOR ───────────────────────────────────────────────────────────

void checkWifi() {
  if (WiFi.status() == WL_CONNECTED) {
    if (wifiConsecFailures > 0) {
      LOG_WIFI("Reconnected, IP=" + WiFi.localIP().toString());
      wifiConsecFailures = 0;
      // Re-enter discovery
      transitionTo(STATE_DISCONNECTED, "wifi_reconnected");
      resetDiscoveryBackoff();
    }
    return;
  }

  unsigned long now = millis();
  if (now - lastWifiCheckMs < WIFI_RECONNECT_INTERVAL_MS) return;
  lastWifiCheckMs = now;

  wifiConsecFailures++;
  LOG_WIFI("Disconnected, attempt " + String(wifiConsecFailures) +
           "/" + String(WIFI_MAX_FAILURES));

  if (wifiConsecFailures >= WIFI_MAX_FAILURES) {
    LOG_WIFI("Max failures reached — restarting chip");
    delay(100);
    ESP.restart();
  }

  // Non-blocking reconnect
  transitionTo(STATE_DISCONNECTED, "wifi_lost");
  WiFi.reconnect();
}

// ─── LOOP WATCHDOG ──────────────────────────────────────────────────────────

void checkLoopWatchdog() {
  unsigned long elapsed = millis() - loopStartMs;
  if (elapsed > LOOP_WDT_WARN_MS) {
    LOG_WDT("loop iteration " + String(elapsed) + "ms (>" +
            String(LOOP_WDT_WARN_MS) + "ms)");
  }
}

// ─── SETUP ──────────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("═══════════════════════════════════════");
  Serial.println(" Wemos D1 Relay Slave");
  Serial.println(" ID:       " + String(SLAVE_ID));
  Serial.println(" Channels: " + String(NUM_CHANNELS));
  Serial.println("═══════════════════════════════════════");

  // Seed random for jitter
  randomSeed(analogRead(A0) ^ micros());

  // Init relays
  for (int i = 0; i < NUM_CHANNELS; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    relayState[i]    = 0;
    relayMode[i]     = 0;
    relayValue[i]    = 0;
    pendingReqId[i]  = "";
    pendingStatus[i] = false;
    applyRelayState(i, 0);
  }

  // WiFi — blocking initial connect with bounded timeout
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID_CFG, WIFI_PASS_CFG);
  LOG_WIFI("Connecting to " + String(WIFI_SSID_CFG));

  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    ESP.wdtFeed();  // feed HW watchdog
    if (millis() - wifiStart > 30000) {
      LOG_WIFI("Initial connect timeout — restarting");
      delay(100);
      ESP.restart();
    }
  }
  LOG_WIFI("Connected IP=" + WiFi.localIP().toString());

  // UDP
  udp.begin(UDP_DISCOVERY_PORT);
  sendUdpDiscovery();
  lastDiscoveryMs = millis();

  // HTTP server
  httpServer.on("/", HTTP_GET, []() {
    if (httpServer.hasArg("usrcmd")) { handleCommand(); return; }
    if (httpServer.hasArg("usrini")) { handleStatusRequest(); return; }
    if (httpServer.hasArg("ping"))   { handlePing(); return; }
    httpServer.send(200, "text/plain", "ok");
  });
  httpServer.onNotFound([]() {
    Serial.println("HTTP 404: " + httpServer.uri());
    httpServer.send(404, "text/plain", "not_found");
  });
  httpServer.begin();

  // Initial registration
  transitionTo(STATE_DISCOVERING, "boot");
  sendRegistration();

  LOG_STATE("Setup complete, entering loop");
}

// ─── MAIN LOOP ──────────────────────────────────────────────────────────────

void loop() {
  loopStartMs = millis();
  ESP.wdtFeed();  // Feed ESP8266 hardware watchdog

  // WiFi health — top priority
  checkWifi();
  if (WiFi.status() != WL_CONNECTED) {
    delay(10);
    checkLoopWatchdog();
    return;   // skip everything until WiFi is back
  }

  // Network I/O
  httpServer.handleClient();
  handleUdpAnnouncements();
  processPendingStatus();

  // Periodic maintenance
  ensureRegistration();
  ensureDiscovery();
  checkMasterStale();

  // Watchdog check
  checkLoopWatchdog();
}
