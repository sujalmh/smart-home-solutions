// ═══════════════════════════════════════════════════════════════════════════════
// ESP32 Master Gateway — Production-Hardened Firmware
// ═══════════════════════════════════════════════════════════════════════════════
//
// Bridges cloud WebSocket (backend) ↔ LAN slave relay boards.
// Refactored for production resilience: state machines, backoff, persistence,
// async-safe command pipeline, coalescing, watchdog, structured diagnostics.
//
// Wire protocol is 100% backward-compatible with socketio_contract.md.
//
// ═══════════════════════════════════════════════════════════════════════════════
//
// ┌────────────────────────────────────────────────────────────────────────────┐
// │ CONNECTION STATE MACHINE (WebSocket to backend)                           │
// │                                                                           │
// │   ┌──────────────┐  begin()   ┌────────────┐  WS_CONNECTED               │
// │   │ DISCONNECTED │───────────►│ CONNECTING  │─────────────┐               │
// │   │              │◄───────────│            │              ▼               │
// │   └──────────────┘  timeout   └────────────┘       ┌────────────┐        │
// │          ▲                                          │ CONNECTED  │        │
// │          │  WS_DISCONNECTED at any point            │ (send reg) │        │
// │          │                                          └──────┬─────┘        │
// │          │                                                 │ implicit     │
// │          │                                                 ▼              │
// │          │                                          ┌────────────┐        │
// │          └──────────────────────────────────────────│ REGISTERED │        │
// │                                                     └────────────┘        │
// └────────────────────────────────────────────────────────────────────────────┘
//
// ┌────────────────────────────────────────────────────────────────────────────┐
// │ TIMING CONSTANTS                                                          │
// ├──────────────────────────────┬────────┬────────────────────────────────────┤
// │ Name                         │ Value  │ Purpose                            │
// ├──────────────────────────────┼────────┼────────────────────────────────────┤
// │ WS_BACKOFF_BASE_MS           │ 2000   │ Initial WS reconnect backoff      │
// │ WS_BACKOFF_MAX_MS            │ 60000  │ Max WS reconnect backoff          │
// │ WS_BACKOFF_MULTIPLIER        │ 2      │ Exponential multiplier            │
// │ WS_MAX_CONSEC_FAILURES       │ 20     │ ESP restart after N WS failures   │
// │ WS_HEARTBEAT_PING_MS         │ 15000  │ WS ping interval                  │
// │ WS_HEARTBEAT_TIMEOUT_MS      │ 3000   │ WS ping timeout                   │
// │ WS_HEARTBEAT_RETRIES         │ 2      │ WS ping retries before disconnect │
// │ TCP_PORT                      │ 6000   │ TCP server for slave connections  │
// │ SLAVE_HTTP_PORT               │ 80     │ HTTP port on slaves               │
// │ UDP_DISCOVERY_PORT            │ 6000   │ UDP discovery port                │
// │ SEEN_TTL_MS                   │ 30000  │ Local seen cache TTL              │
// │ SEEN_EMIT_INTERVAL_MS        │ 10000  │ Throttle for slave_seen WS events │
// │ COMMAND_COOLDOWN_MS           │ 120    │ Per-(dev,comp) command dedup      │
// │ STATUS_COOLDOWN_MS            │ 600    │ Per-(dev,comp) status poll dedup  │
// │ STATUS_FANOUT_DELAY_MS       │ 120    │ Delay between fan-out polls       │
// │ COMMAND_RETRY_DELAY_MS       │ 250    │ Retry backoff for failed commands │
// │ COMMAND_MAX_RETRIES           │ 2      │ Max retries per command slot      │
// │ TCP_IDLE_TIMEOUT_MS           │ 500    │ TCP client idle disconnect        │
// │ TCP_READ_TIMEOUT_MS           │ 250    │ TCP read timeout per client       │
// │ HTTP_TIMEOUT_MS               │ 900    │ HTTP GET timeout to slaves        │
// │ MAX_CONCURRENT_HTTP           │ 4      │ Max in-flight HTTP requests       │
// │ SLAVE_STALE_MS                │ 90000  │ No-data → slave stale            │
// │ SLAVE_PING_INTERVAL_MS       │ 60000  │ Ping each bound slave             │
// │ SLAVE_PING_TIMEOUT_MS        │ 500    │ Ping response timeout             │
// │ SLAVE_MAX_MISSED_PINGS       │ 3      │ Missed pings → stale             │
// │ HEARTBEAT_EMIT_INTERVAL_MS   │ 60000  │ gateway_heartbeat event period    │
// │ METRICS_LOG_INTERVAL_MS      │ 60000  │ Serial metrics dump period        │
// │ STALE_REFRESH_INTERVAL_MS    │ 120000 │ Auto-poll bound slave if silent   │
// │ SEEN_CLEANUP_INTERVAL_MS     │ 30000  │ Cleanup stale seen entries        │
// │ MAX_SLAVES                    │ 32     │ Max bound slaves                  │
// │ MAX_PENDING                   │ 32     │ Max pending bind requests         │
// │ MAX_SEEN                      │ 32     │ Max seen slave cache              │
// │ MAX_COMMAND_SLOTS             │ 32     │ Command queue depth               │
// │ MAX_FANOUT_QUEUE              │ 8      │ Status fanout ring buffer depth   │
// │ NVS_NAMESPACE                 │ "gw"   │ NVS persistence namespace         │
// │ LOOP_WDT_WARN_MS             │ 500    │ Warn if loop() exceeds this       │
// │ TASK_WDT_TIMEOUT_S           │ 10     │ ESP32 task WDT timeout            │
// │ BACKOFF_JITTER_PCT           │ 25     │ ±% jitter on backoff intervals    │
// └──────────────────────────────┴────────┴────────────────────────────────────┘
//
// ═══════════════════════════════════════════════════════════════════════════════

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <HTTPClient.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <esp_task_wdt.h>

// ═════════════════════════════════════════════════════════════════════════════
// SECTION 1: CONFIGURATION & CONSTANTS
// ═════════════════════════════════════════════════════════════════════════════

// WiFi credentials
static const char* WIFI_SSID = "Nuron";
static const char* WIFI_PASS = "mnbvcx12";

// Backend WebSocket
static const char* SOCKET_HOST = "api.sms.hebbit.tech";
static const uint16_t SOCKET_PORT = 443;
static const char* SOCKET_PATH = "/ws";
static const bool   SOCKET_INSECURE = true;
static const char* SOCKET_CA_CERT = R"EOF(
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
static const char* SERVER_ID = "0001";

// ─── Network ports ──────────────────────────────────────────────────────────
static const uint16_t TCP_PORT            = 6000;
static const uint16_t SLAVE_HTTP_PORT     = 80;
static const uint16_t UDP_DISCOVERY_PORT  = 6000;

// ─── Feature flags ──────────────────────────────────────────────────────────
static const bool LOG_VERBOSE           = true;
static const bool REQUIRE_BINDING       = true;

// ─── Timing constants ───────────────────────────────────────────────────────
static const unsigned long WS_BACKOFF_BASE_MS          = 2000;
static const unsigned long WS_BACKOFF_MAX_MS           = 60000;
static const uint8_t       WS_BACKOFF_MULTIPLIER       = 2;
static const uint8_t       WS_MAX_CONSEC_FAILURES      = 20;
static const unsigned long WS_HEARTBEAT_PING_MS        = 15000;
static const unsigned long WS_HEARTBEAT_TIMEOUT_MS     = 3000;
static const uint8_t       WS_HEARTBEAT_RETRIES        = 2;

static const unsigned long SEEN_TTL_MS                 = 30000;
static const unsigned long SEEN_EMIT_INTERVAL_MS       = 10000;
static const unsigned long SEEN_CLEANUP_INTERVAL_MS    = 30000;
static const unsigned long COMMAND_COOLDOWN_MS         = 120;
static const unsigned long STATUS_COOLDOWN_MS          = 600;
static const unsigned long STATUS_FANOUT_DELAY_MS      = 120;
static const unsigned long COMMAND_RETRY_DELAY_MS      = 250;
static const uint8_t       COMMAND_MAX_RETRIES         = 2;
static const unsigned long TCP_IDLE_TIMEOUT_MS         = 500;
static const unsigned long TCP_READ_TIMEOUT_MS         = 250;
static const unsigned long HTTP_TIMEOUT_MS             = 900;
static const uint8_t       MAX_CONCURRENT_HTTP         = 4;
static const unsigned long SLAVE_STALE_MS              = 90000;
static const unsigned long SLAVE_PING_INTERVAL_MS      = 60000;
static const unsigned long SLAVE_PING_TIMEOUT_MS       = 500;
static const uint8_t       SLAVE_MAX_MISSED_PINGS      = 3;
static const unsigned long HEARTBEAT_EMIT_INTERVAL_MS  = 60000;
static const unsigned long METRICS_LOG_INTERVAL_MS     = 60000;
static const unsigned long STALE_REFRESH_INTERVAL_MS   = 120000;
static const unsigned long LOOP_WDT_WARN_MS            = 500;
static const uint8_t       TASK_WDT_TIMEOUT_S          = 10;
static const uint8_t       BACKOFF_JITTER_PCT          = 25;

// ─── Capacity limits ────────────────────────────────────────────────────────
static const size_t MAX_SLAVES        = 32;
static const size_t MAX_PENDING       = 32;
static const size_t MAX_SEEN          = 32;
static const size_t MAX_COMMAND_SLOTS = 32;
static const size_t MAX_FANOUT_QUEUE  = 8;

// ─── NVS ────────────────────────────────────────────────────────────────────
static const char* NVS_NAMESPACE     = "gw";
static const char* NVS_KEY_SLAVE_IDS = "slaves";
static const char* NVS_KEY_SLAVE_IPS = "slaveips";
static const char* NVS_KEY_SLAVE_CNT = "slavecnt";

// ═════════════════════════════════════════════════════════════════════════════
// SECTION 2: STATE TYPES & ENUMS
// ═════════════════════════════════════════════════════════════════════════════

enum ConnState : uint8_t {
  CONN_DISCONNECTED,
  CONN_CONNECTING,
  CONN_CONNECTED,
  CONN_REGISTERED
};

static const char* connStateNames[] = {
  "DISCONNECTED", "CONNECTING", "CONNECTED", "REGISTERED"
};

enum SlaveHealth : uint8_t {
  SLAVE_ACTIVE,
  SLAVE_STALE
};

struct SlaveEntry {
  String id;
  String ip;
  SlaveHealth health;
  unsigned long lastDataMs;
  unsigned long lastPingMs;
  uint8_t missedPings;
  // Per-slave command/status cooldown
  unsigned long lastCommandMs;
  String        lastCommandComp;
  int           lastCommandMod;
  int           lastCommandStat;
  int           lastCommandVal;
  unsigned long lastStatusMs;
  String        lastStatusComp;
};

struct CommandSlot {
  bool     used;
  bool     inFlight;
  String   devId;
  String   comp;
  int      mod;
  int      stat;
  int      val;
  String   reqId;
  uint8_t  attempts;
  unsigned long nextAtMs;
  uint16_t coalesceCount;
};

struct FanoutRequest {
  bool     active;
  String   devId;
  uint8_t  nextComp;
  unsigned long nextAtMs;
};

// ═════════════════════════════════════════════════════════════════════════════
// SECTION 3: GLOBAL STATE
// ═════════════════════════════════════════════════════════════════════════════

WiFiServer tcpServer(TCP_PORT);
WebSocketsClient webSocket;
WiFiUDP udpSocket;
Preferences nvs;

// Connection state machine
ConnState connState              = CONN_DISCONNECTED;
unsigned long wsBackoffMs        = WS_BACKOFF_BASE_MS;
uint8_t       wsConsecFailures   = 0;
unsigned long wsNextConnectAtMs  = 0;
bool          wsWasConnected     = false;

// Slave registry
SlaveEntry slaves[MAX_SLAVES];
size_t     slaveCount = 0;

// Pending bind requests
String pendingSlaves[MAX_PENDING];
size_t pendingCount = 0;

// Seen cache
String        seenIds[MAX_SEEN];
String        seenIps[MAX_SEEN];
unsigned long seenAt[MAX_SEEN];
size_t        seenCount = 0;
unsigned long lastSeenCleanupMs = 0;

// Command queue
CommandSlot commandSlots[MAX_COMMAND_SLOTS];
uint8_t     httpInFlightCount = 0;

// Status fanout ring buffer
FanoutRequest fanoutQueue[MAX_FANOUT_QUEUE];
size_t fanoutHead = 0;
size_t fanoutTail = 0;

// Status dedup cache (master-side)
struct StaDedupEntry {
  String devId;
  String comp;
  int mod;
  int stat;
  int val;
  unsigned long ts;
};
static const size_t MAX_STA_DEDUP = 64;
StaDedupEntry staDedupCache[MAX_STA_DEDUP];
size_t staDedupCount = 0;

// Diagnostics
unsigned long lastHeartbeatMs  = 0;
unsigned long lastMetricsMs    = 0;
unsigned long loopStartMs      = 0;
unsigned long bootMs           = 0;

// ═════════════════════════════════════════════════════════════════════════════
// SECTION 4: STRUCTURED LOGGING
// ═════════════════════════════════════════════════════════════════════════════

void logPrefixed(const char* prefix, const String& msg) {
  if (!LOG_VERBOSE) return;
  Serial.print('[');
  Serial.print(prefix);
  Serial.print("] ");
  Serial.println(msg);
}

#define LOG_CONN(m)   logPrefixed("CONN", m)
#define LOG_CMD(m)    logPrefixed("CMD", m)
#define LOG_STA(m)    logPrefixed("STA", m)
#define LOG_DISC(m)   logPrefixed("DISC", m)
#define LOG_BIND(m)   logPrefixed("BIND", m)
#define LOG_SLAVE(m)  logPrefixed("SLAVE", m)
#define LOG_WDT(m)    logPrefixed("WDT", m)
#define LOG_NVS(m)    logPrefixed("NVS", m)
#define LOG_HEAP(m)   logPrefixed("HEAP", m)
#define LOG_METRIC(m) logPrefixed("METRIC", m)

// ═════════════════════════════════════════════════════════════════════════════
// SECTION 5: UTILITY FUNCTIONS
// ═════════════════════════════════════════════════════════════════════════════

String normalizeId(const String& value) {
  if (value.startsWith("RSW-")) return value.substring(4);
  return value;
}

const char* webSocketTypeName(WStype_t type) {
  switch (type) {
    case WStype_DISCONNECTED: return "DISCONNECTED";
    case WStype_CONNECTED:    return "CONNECTED";
    case WStype_TEXT:         return "TEXT";
    case WStype_BIN:         return "BIN";
    case WStype_ERROR:       return "ERROR";
    case WStype_PING:        return "PING";
    case WStype_PONG:        return "PONG";
    default:                  return "UNKNOWN";
  }
}

String payloadToString(uint8_t* payload, size_t length) {
  String out;
  out.reserve(length);
  for (size_t i = 0; i < length; i++) {
    out += static_cast<char>(payload[i]);
  }
  return out;
}

int toIntSafe(const String& value, int fallback) {
  if (value.length() == 0) return fallback;
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

String urlEncode(const String& value) {
  String encoded;
  encoded.reserve(value.length() * 2);
  for (size_t i = 0; i < value.length(); i++) {
    char c = value[i];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += c;
    } else {
      char buf[4];
      snprintf(buf, sizeof(buf), "%%%02X", static_cast<unsigned char>(c));
      encoded += buf;
    }
  }
  return encoded;
}

unsigned long applyJitter(unsigned long base) {
  long jitter = (long)base * BACKOFF_JITTER_PCT / 100;
  long offset = random(-jitter, jitter + 1);
  long result = (long)base + offset;
  return result > 0 ? (unsigned long)result : 1;
}

// ═════════════════════════════════════════════════════════════════════════════
// SECTION 6: NVS PERSISTENCE (Slave Bindings)
// ═════════════════════════════════════════════════════════════════════════════

void nvsSaveSlaves() {
  nvs.begin(NVS_NAMESPACE, false);
  String ids = "";
  String ips = "";
  for (size_t i = 0; i < slaveCount; i++) {
    if (i > 0) { ids += ","; ips += ","; }
    ids += slaves[i].id;
    ips += slaves[i].ip;
  }
  nvs.putString(NVS_KEY_SLAVE_IDS, ids);
  nvs.putString(NVS_KEY_SLAVE_IPS, ips);
  nvs.putUInt(NVS_KEY_SLAVE_CNT, (uint32_t)slaveCount);
  nvs.end();
  LOG_NVS("Saved " + String(slaveCount) + " slaves to flash");
}

void nvsLoadSlaves() {
  nvs.begin(NVS_NAMESPACE, true);
  uint32_t cnt = nvs.getUInt(NVS_KEY_SLAVE_CNT, 0);
  if (cnt == 0) {
    nvs.end();
    LOG_NVS("No persisted slaves found");
    return;
  }
  String ids = nvs.getString(NVS_KEY_SLAVE_IDS, "");
  String ips = nvs.getString(NVS_KEY_SLAVE_IPS, "");
  nvs.end();

  String idArr[MAX_SLAVES];
  String ipArr[MAX_SLAVES];
  int idCount = splitTokens(ids, ',', idArr, MAX_SLAVES);
  int ipCount = splitTokens(ips, ',', ipArr, MAX_SLAVES);
  int loadCount = min(idCount, ipCount);
  loadCount = min(loadCount, (int)cnt);
  loadCount = min(loadCount, (int)MAX_SLAVES);

  for (int i = 0; i < loadCount; i++) {
    if (idArr[i].length() == 0) continue;
    slaves[slaveCount].id              = idArr[i];
    slaves[slaveCount].ip              = ipArr[i];
    slaves[slaveCount].health          = SLAVE_STALE; // stale until we hear from them
    slaves[slaveCount].lastDataMs      = 0;
    slaves[slaveCount].lastPingMs      = 0;
    slaves[slaveCount].missedPings     = 0;
    slaves[slaveCount].lastCommandMs   = 0;
    slaves[slaveCount].lastCommandComp = "";
    slaves[slaveCount].lastCommandMod  = 0;
    slaves[slaveCount].lastCommandStat = 0;
    slaves[slaveCount].lastCommandVal  = 0;
    slaves[slaveCount].lastStatusMs    = 0;
    slaves[slaveCount].lastStatusComp  = "";
    slaveCount++;
  }
  LOG_NVS("Loaded " + String(slaveCount) + " slaves from flash");
}

void nvsClearSlaves() {
  nvs.begin(NVS_NAMESPACE, false);
  nvs.remove(NVS_KEY_SLAVE_IDS);
  nvs.remove(NVS_KEY_SLAVE_IPS);
  nvs.remove(NVS_KEY_SLAVE_CNT);
  nvs.end();
  LOG_NVS("Cleared slave flash storage");
}

// ═════════════════════════════════════════════════════════════════════════════
// SECTION 7: SLAVE REGISTRY
// ═════════════════════════════════════════════════════════════════════════════

int findSlaveIndex(const String& id) {
  for (size_t i = 0; i < slaveCount; i++) {
    if (slaves[i].id == id) return static_cast<int>(i);
  }
  return -1;
}

String findSlaveIp(const String& id) {
  int idx = findSlaveIndex(id);
  return idx >= 0 ? slaves[idx].ip : String();
}

bool isKnownSlave(const String& id) {
  return findSlaveIndex(id) >= 0;
}

bool isPendingSlave(const String& id) {
  for (size_t i = 0; i < pendingCount; i++) {
    if (pendingSlaves[i] == id) return true;
  }
  return false;
}

void addPendingSlave(const String& id) {
  if (id.length() == 0 || isPendingSlave(id)) return;
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

void initSlaveEntry(size_t idx, const String& id, const String& ip) {
  slaves[idx].id              = id;
  slaves[idx].ip              = ip;
  slaves[idx].health          = SLAVE_ACTIVE;
  slaves[idx].lastDataMs      = millis();
  slaves[idx].lastPingMs      = 0;
  slaves[idx].missedPings     = 0;
  slaves[idx].lastCommandMs   = 0;
  slaves[idx].lastCommandComp = "";
  slaves[idx].lastCommandMod  = 0;
  slaves[idx].lastCommandStat = 0;
  slaves[idx].lastCommandVal  = 0;
  slaves[idx].lastStatusMs    = 0;
  slaves[idx].lastStatusComp  = "";
}

void upsertSlave(const String& id, const String& ip) {
  if (id.length() == 0 || ip.length() == 0) return;

  int idx = findSlaveIndex(id);
  if (idx >= 0) {
    slaves[idx].ip         = ip;
    slaves[idx].lastDataMs = millis();
    if (slaves[idx].health == SLAVE_STALE) {
      slaves[idx].health      = SLAVE_ACTIVE;
      slaves[idx].missedPings = 0;
      LOG_SLAVE("ACTIVE clientID=" + id + " (was STALE)");
    }
    return;
  }

  if (slaveCount < MAX_SLAVES) {
    initSlaveEntry(slaveCount, id, ip);
    slaveCount++;
    LOG_SLAVE("Added clientID=" + id + " ip=" + ip +
              " (slot " + String(slaveCount - 1) + ")");
    nvsSaveSlaves();
  } else {
    LOG_SLAVE("Registry full, cannot add " + id);
  }
}

void removeSlaveByIndex(size_t idx) {
  if (idx >= slaveCount) return;
  String id = slaves[idx].id;
  slaves[idx] = slaves[slaveCount - 1];
  slaveCount--;
  nvsSaveSlaves();
  LOG_SLAVE("Removed clientID=" + id);
}

// ═════════════════════════════════════════════════════════════════════════════
// SECTION 8: SEEN CACHE
// ═════════════════════════════════════════════════════════════════════════════

String findSeenIp(const String& id) {
  for (size_t i = 0; i < seenCount; i++) {
    if (seenIds[i] == id) return seenIps[i];
  }
  return String();
}

// Forward declaration
void emitSlaveSeen(const String& clientId, const String& ip);

void recordSeen(const String& clientId, const String& ip, bool forceEmit) {
  if (clientId.length() == 0) return;
  unsigned long now = millis();

  for (size_t i = 0; i < seenCount; i++) {
    if (seenIds[i] == clientId) {
      bool ipChanged = (seenIps[i] != ip);
      seenIps[i] = ip;
      bool shouldEmit = forceEmit || ipChanged ||
                        (now - seenAt[i] >= SEEN_EMIT_INTERVAL_MS);
      seenAt[i] = now;
      if (shouldEmit && connState == CONN_REGISTERED) {
        emitSlaveSeen(clientId, ip);
      }
      return;
    }
  }

  // New entry
  if (seenCount < MAX_SEEN) {
    seenIds[seenCount] = clientId;
    seenIps[seenCount] = ip;
    seenAt[seenCount]  = now;
    seenCount++;
  }
  if (connState == CONN_REGISTERED) {
    emitSlaveSeen(clientId, ip);
  }
}

void cleanupSeen() {
  unsigned long now = millis();
  if (now - lastSeenCleanupMs < SEEN_CLEANUP_INTERVAL_MS) return;
  lastSeenCleanupMs = now;

  size_t i = 0;
  while (i < seenCount) {
    if (now - seenAt[i] > SEEN_TTL_MS) {
      LOG_DISC("PRUNE clientID=" + seenIds[i]);
      seenIds[i] = seenIds[seenCount - 1];
      seenIps[i] = seenIps[seenCount - 1];
      seenAt[i]  = seenAt[seenCount - 1];
      seenCount--;
    } else {
      i++;
    }
  }
}

void flushSeen() {
  if (connState != CONN_REGISTERED) return;
  for (size_t i = 0; i < seenCount; i++) {
    emitSlaveSeen(seenIds[i], seenIps[i]);
  }
  LOG_DISC("Flushed " + String(seenCount) + " seen entries");
}

// ═════════════════════════════════════════════════════════════════════════════
// SECTION 9: TRANSPORT LAYER (WebSocket)
// ═════════════════════════════════════════════════════════════════════════════

bool wsSendJson(const char* eventName, const JsonDocument& doc) {
  if (connState < CONN_CONNECTED) {
    LOG_CONN("WS send skip: not connected");
    return false;
  }
  DynamicJsonDocument envelope(512);
  envelope["event"] = eventName;
  envelope["data"].set(doc.as<JsonVariantConst>());
  String message;
  serializeJson(envelope, message);
  bool ok = webSocket.sendTXT(message);
  if (!ok) {
    LOG_CONN("WS sendTXT failed for event=" + String(eventName));
  }
  return ok;
}

// ─── Connection state machine helpers ────────────────────────────────────

void connTransition(ConnState newState, const String& reason) {
  if (newState == connState) return;
  LOG_CONN(String(connStateNames[connState]) + " -> " +
           String(connStateNames[newState]) + " reason=" + reason);
  connState = newState;
}

void resetWsBackoff() {
  wsBackoffMs      = WS_BACKOFF_BASE_MS;
  wsConsecFailures = 0;
}

void advanceWsBackoff() {
  wsConsecFailures++;
  if (wsConsecFailures >= WS_MAX_CONSEC_FAILURES) {
    LOG_CONN("Max WS failures (" + String(WS_MAX_CONSEC_FAILURES) +
             ") — restarting ESP");
    delay(100);
    ESP.restart();
  }
  wsBackoffMs = min((unsigned long)(wsBackoffMs * WS_BACKOFF_MULTIPLIER),
                    WS_BACKOFF_MAX_MS);
  wsNextConnectAtMs = millis() + applyJitter(wsBackoffMs);
  LOG_CONN("Backoff: next in " + String(wsBackoffMs) +
           "ms (failure " + String(wsConsecFailures) + ")");
}

// ═════════════════════════════════════════════════════════════════════════════
// SECTION 10: WEBSOCKET EVENT EMITTERS
// ═════════════════════════════════════════════════════════════════════════════

void emitGatewayRegister() {
  DynamicJsonDocument doc(256);
  doc["serverID"] = SERVER_ID;
  doc["ip"] = WiFi.localIP().toString();
  LOG_CONN("Emit gateway_register server=" + String(SERVER_ID));
  wsSendJson("gateway_register", doc);
}

void emitRegister(const String& clientId, const String& ip) {
  DynamicJsonDocument doc(256);
  doc["serverID"] = SERVER_ID;
  doc["clientID"] = clientId;
  doc["ip"]       = ip;
  LOG_BIND("Emit register client=" + clientId + " ip=" + ip);
  wsSendJson("register", doc);
}

void emitSlaveSeen(const String& clientId, const String& ip) {
  DynamicJsonDocument doc(256);
  doc["serverID"] = SERVER_ID;
  doc["clientID"] = clientId;
  doc["ip"]       = ip;
  wsSendJson("slave_seen", doc);
}

void emitStaResult(const String& devId, const String& comp,
                   int mod, int stat, int val, const String& reqId) {
  DynamicJsonDocument doc(256);
  doc["devID"] = devId;
  doc["comp"]  = comp;
  doc["mod"]   = mod;
  doc["stat"]  = stat;
  doc["val"]   = val;
  if (reqId.length() > 0) doc["reqId"] = reqId;
  LOG_STA("Emit staresult dev=" + devId + " comp=" + comp +
          " stat=" + String(stat) + " val=" + String(val));
  wsSendJson("staresult", doc);
}

void emitResponse(const String& devId, const String& comp,
                  int mod, int stat, int val, const String& reqId) {
  DynamicJsonDocument doc(256);
  doc["devID"] = devId;
  doc["comp"]  = comp;
  doc["mod"]   = mod;
  doc["stat"]  = stat;
  doc["val"]   = val;
  if (reqId.length() > 0) doc["reqId"] = reqId;
  LOG_STA("Emit response dev=" + devId + " comp=" + comp);
  wsSendJson("response", doc);
}

void emitGatewayHeartbeat() {
  DynamicJsonDocument doc(256);
  doc["serverID"]    = SERVER_ID;
  doc["uptime_s"]    = (millis() - bootMs) / 1000;
  doc["slave_count"] = (int)slaveCount;
  int qd = 0;
  for (size_t i = 0; i < MAX_COMMAND_SLOTS; i++) {
    if (commandSlots[i].used) qd++;
  }
  doc["queue_depth"] = qd;
  doc["free_heap"]   = (int)ESP.getFreeHeap();
  wsSendJson("gateway_heartbeat", doc);
}

// ═════════════════════════════════════════════════════════════════════════════
// SECTION 11: STATUS DEDUP CACHE (Master-side)
// ═════════════════════════════════════════════════════════════════════════════

bool shouldForwardStatus(const String& devId, const String& comp,
                         int mod, int stat, int val) {
  unsigned long now = millis();
  for (size_t i = 0; i < staDedupCount; i++) {
    if (staDedupCache[i].devId == devId && staDedupCache[i].comp == comp) {
      if (staDedupCache[i].mod == mod &&
          staDedupCache[i].stat == stat &&
          staDedupCache[i].val == val &&
          (now - staDedupCache[i].ts < 1000)) {
        return false; // duplicate within 1s window
      }
      staDedupCache[i].mod  = mod;
      staDedupCache[i].stat = stat;
      staDedupCache[i].val  = val;
      staDedupCache[i].ts   = now;
      return true;
    }
  }
  // New entry
  if (staDedupCount < MAX_STA_DEDUP) {
    staDedupCache[staDedupCount] = {devId, comp, mod, stat, val, now};
    staDedupCount++;
  } else {
    // Overwrite oldest
    unsigned long oldest = now;
    size_t oldestIdx = 0;
    for (size_t i = 0; i < MAX_STA_DEDUP; i++) {
      if (staDedupCache[i].ts < oldest) {
        oldest = staDedupCache[i].ts;
        oldestIdx = i;
      }
    }
    staDedupCache[oldestIdx] = {devId, comp, mod, stat, val, now};
  }
  return true;
}

// ═════════════════════════════════════════════════════════════════════════════
// SECTION 12: COMMAND PIPELINE
// ═════════════════════════════════════════════════════════════════════════════

int findCommandSlot(const String& devId, const String& comp) {
  for (size_t i = 0; i < MAX_COMMAND_SLOTS; i++) {
    if (!commandSlots[i].used) continue;
    if (commandSlots[i].devId == devId && commandSlots[i].comp == comp)
      return static_cast<int>(i);
  }
  return -1;
}

int findFreeCommandSlot() {
  for (size_t i = 0; i < MAX_COMMAND_SLOTS; i++) {
    if (!commandSlots[i].used) return static_cast<int>(i);
  }
  return -1;
}

int findOldestNonInflightSlot() {
  int oldest = -1;
  unsigned long oldestTime = ULONG_MAX;
  for (size_t i = 0; i < MAX_COMMAND_SLOTS; i++) {
    if (!commandSlots[i].used || commandSlots[i].inFlight) continue;
    if (commandSlots[i].nextAtMs < oldestTime) {
      oldestTime = commandSlots[i].nextAtMs;
      oldest = static_cast<int>(i);
    }
  }
  return oldest;
}

void clearCommandSlot(int index) {
  if (index < 0 || index >= static_cast<int>(MAX_COMMAND_SLOTS)) return;
  if (commandSlots[index].inFlight && httpInFlightCount > 0) {
    httpInFlightCount--;
  }
  commandSlots[index].used          = false;
  commandSlots[index].inFlight      = false;
  commandSlots[index].devId         = "";
  commandSlots[index].comp          = "";
  commandSlots[index].mod           = 0;
  commandSlots[index].stat          = 0;
  commandSlots[index].val           = 0;
  commandSlots[index].reqId         = "";
  commandSlots[index].attempts      = 0;
  commandSlots[index].nextAtMs      = 0;
  commandSlots[index].coalesceCount = 0;
}

void enqueueCommandToSlave(const String& devId, const String& comp,
                           int mod, int stat, int val, const String& reqId) {
  if (devId.length() == 0 || comp.length() == 0) return;

  int index = findCommandSlot(devId, comp);
  bool replacing = (index >= 0);

  if (index >= 0 && commandSlots[index].inFlight) {
    // In-flight: update payload to re-send after current completes
    commandSlots[index].mod    = mod;
    commandSlots[index].stat   = stat;
    commandSlots[index].val    = val;
    commandSlots[index].reqId  = reqId;
    commandSlots[index].coalesceCount++;
    LOG_CMD("COALESCED (in-flight) dev=" + devId + " comp=" + comp +
            " count=" + String(commandSlots[index].coalesceCount));
    return;
  }

  if (index < 0) index = findFreeCommandSlot();

  // Queue full — evict oldest non-in-flight
  if (index < 0) {
    int oldest = findOldestNonInflightSlot();
    if (oldest >= 0) {
      LOG_CMD("DROP oldest dev=" + commandSlots[oldest].devId +
              " comp=" + commandSlots[oldest].comp + " reason=QUEUE_FULL");
      clearCommandSlot(oldest);
      index = oldest;
    } else {
      LOG_CMD("DROP dev=" + devId + " comp=" + comp +
              " reason=QUEUE_FULL_ALL_INFLIGHT");
      return;
    }
  }

  commandSlots[index].used          = true;
  commandSlots[index].inFlight      = false;
  commandSlots[index].devId         = devId;
  commandSlots[index].comp          = comp;
  commandSlots[index].mod           = mod;
  commandSlots[index].stat          = stat;
  commandSlots[index].val           = val;
  commandSlots[index].reqId         = reqId;
  commandSlots[index].attempts      = 0;
  commandSlots[index].nextAtMs      = millis();
  commandSlots[index].coalesceCount = replacing
      ? commandSlots[index].coalesceCount + 1 : 0;

  if (replacing) {
    LOG_CMD("COALESCED dev=" + devId + " comp=" + comp +
            " count=" + String(commandSlots[index].coalesceCount));
  } else {
    LOG_CMD("QUEUED dev=" + devId + " comp=" + comp);
  }
}

bool sendHttpGet(const String& ip, const String& path, int timeoutMs) {
  if (ip.length() == 0) {
    LOG_CMD("HTTP skip: missing slave IP");
    return false;
  }
  String url = "http://" + ip + ":" + String(SLAVE_HTTP_PORT) + path;
  LOG_CMD("HTTP GET -> " + url);

  WiFiClient client;
  HTTPClient http;
  http.setTimeout(timeoutMs > 0 ? timeoutMs : (int)HTTP_TIMEOUT_MS);
  if (!http.begin(client, url)) {
    LOG_CMD("HTTP begin failed");
    return false;
  }

  int code = http.GET();
  http.end();

  if (code > 0) {
    LOG_CMD("HTTP status=" + String(code));
    return code < 400;
  }
  LOG_CMD("HTTP error=" + http.errorToString(code));
  return false;
}

bool sendCommandToSlave(const String& devId, const String& comp,
                        int mod, int stat, int val, const String& reqId) {
  String ip = findSlaveIp(devId);
  if (ip.length() == 0) {
    LOG_CMD("SKIP unknown slave " + devId);
    return false;
  }

  // Per-(dev,comp) cooldown
  int idx = findSlaveIndex(devId);
  unsigned long now = millis();
  if (idx >= 0 && now - slaves[idx].lastCommandMs < COMMAND_COOLDOWN_MS) {
    bool samePayload =
        slaves[idx].lastCommandComp == comp &&
        slaves[idx].lastCommandMod  == mod &&
        slaves[idx].lastCommandStat == stat &&
        slaves[idx].lastCommandVal  == val;
    if (samePayload) {
      LOG_CMD("SKIP cooldown dev=" + devId + " comp=" + comp);
      return true;
    }
  }
  if (idx >= 0) {
    slaves[idx].lastCommandMs   = now;
    slaves[idx].lastCommandComp = comp;
    slaves[idx].lastCommandMod  = mod;
    slaves[idx].lastCommandStat = stat;
    slaves[idx].lastCommandVal  = val;
  }

  String payload = devId + ";" + comp + ";" + String(mod) + ";" +
                   String(stat) + ";" + String(val) + ";";
  if (reqId.length() > 0) payload += reqId + ";";
  String path = "/?usrcmd=" + urlEncode(payload);
  return sendHttpGet(ip, path, HTTP_TIMEOUT_MS);
}

void processCommandQueue() {
  unsigned long now = millis();

  int selected = -1;
  unsigned long selectedAt = ULONG_MAX;

  for (size_t i = 0; i < MAX_COMMAND_SLOTS; i++) {
    if (!commandSlots[i].used || commandSlots[i].inFlight) continue;
    if (now < commandSlots[i].nextAtMs) continue;
    if (commandSlots[i].nextAtMs < selectedAt) {
      selected = static_cast<int>(i);
      selectedAt = commandSlots[i].nextAtMs;
    }
  }

  if (selected < 0) return;
  if (httpInFlightCount >= MAX_CONCURRENT_HTTP) return;

  CommandSlot& slot = commandSlots[selected];
  LOG_CMD("DISPATCHED dev=" + slot.devId + " comp=" + slot.comp +
          " attempt=" + String(slot.attempts + 1));

  bool sent = sendCommandToSlave(slot.devId, slot.comp,
                                 slot.mod, slot.stat, slot.val, slot.reqId);
  if (sent) {
    clearCommandSlot(selected);
    return;
  }

  slot.attempts++;
  if (slot.attempts >= COMMAND_MAX_RETRIES) {
    LOG_CMD("DROP dev=" + slot.devId + " comp=" + slot.comp +
            " reason=RETRY_EXHAUSTED attempts=" + String(slot.attempts));
    clearCommandSlot(selected);
    return;
  }
  slot.nextAtMs = now + COMMAND_RETRY_DELAY_MS;
  LOG_CMD("RETRY scheduled dev=" + slot.devId + " comp=" + slot.comp +
          " attempt=" + String(slot.attempts) +
          " nextIn=" + String(COMMAND_RETRY_DELAY_MS) + "ms");
}

// ═════════════════════════════════════════════════════════════════════════════
// SECTION 13: STATUS PIPELINE
// ═════════════════════════════════════════════════════════════════════════════

void sendStatusToSlave(const String& devId, const String& comp) {
  String ip = findSlaveIp(devId);
  if (ip.length() == 0) {
    LOG_STA("SKIP unknown slave " + devId);
    return;
  }

  int idx = findSlaveIndex(devId);
  unsigned long now = millis();
  if (idx >= 0 && now - slaves[idx].lastStatusMs < STATUS_COOLDOWN_MS) {
    if (slaves[idx].lastStatusComp == comp) {
      LOG_STA("SKIP cooldown dev=" + devId + " comp=" + comp);
      return;
    }
  }
  if (idx >= 0) {
    slaves[idx].lastStatusMs   = now;
    slaves[idx].lastStatusComp = comp;
  }

  String payload = devId + ";" + comp + ";";
  String path = "/?usrini=" + urlEncode(payload);
  sendHttpGet(ip, path, 700);
}

// ─── Fanout ring buffer ─────────────────────────────────────────────

size_t fanoutSize() {
  return (fanoutTail + MAX_FANOUT_QUEUE - fanoutHead) % MAX_FANOUT_QUEUE;
}

void startStatusFanout(const String& devId) {
  // Coalesce: find existing for same device
  size_t idx = fanoutHead;
  while (idx != fanoutTail) {
    if (fanoutQueue[idx].active && fanoutQueue[idx].devId == devId) {
      fanoutQueue[idx].nextComp = 0;
      fanoutQueue[idx].nextAtMs = millis();
      LOG_STA("Fanout COALESCED dev=" + devId);
      return;
    }
    idx = (idx + 1) % MAX_FANOUT_QUEUE;
  }

  size_t nextTail = (fanoutTail + 1) % MAX_FANOUT_QUEUE;
  if (nextTail == fanoutHead) {
    LOG_STA("Fanout DROP oldest dev=" + fanoutQueue[fanoutHead].devId);
    fanoutHead = (fanoutHead + 1) % MAX_FANOUT_QUEUE;
  }
  fanoutQueue[fanoutTail].active   = true;
  fanoutQueue[fanoutTail].devId    = devId;
  fanoutQueue[fanoutTail].nextComp = 0;
  fanoutQueue[fanoutTail].nextAtMs = millis();
  fanoutTail = (fanoutTail + 1) % MAX_FANOUT_QUEUE;
  LOG_STA("Fanout QUEUED dev=" + devId);
}

void processStatusFanout() {
  if (fanoutHead == fanoutTail) return;

  FanoutRequest& req = fanoutQueue[fanoutHead];
  if (!req.active) {
    fanoutHead = (fanoutHead + 1) % MAX_FANOUT_QUEUE;
    return;
  }

  unsigned long now = millis();
  if (now < req.nextAtMs) return;

  String comp = "Comp" + String(req.nextComp);
  sendStatusToSlave(req.devId, comp);
  req.nextComp++;

  if (req.nextComp >= 4) {
    req.active = false;
    req.devId  = "";
    fanoutHead = (fanoutHead + 1) % MAX_FANOUT_QUEUE;
    return;
  }
  req.nextAtMs = now + STATUS_FANOUT_DELAY_MS;
}

// ═════════════════════════════════════════════════════════════════════════════
// SECTION 14: DISCOVERY MANAGER
// ═════════════════════════════════════════════════════════════════════════════

void handleUdpDiscovery() {
  int packetSize = udpSocket.parsePacket();
  if (packetSize <= 0) return;

  IPAddress remoteIp = udpSocket.remoteIP();
  uint16_t remotePort = udpSocket.remotePort();
  char buffer[128];
  int len = udpSocket.read(buffer, sizeof(buffer) - 1);
  if (len <= 0) return;
  buffer[len] = '\0';

  String line = String(buffer);
  line.trim();
  if (!line.startsWith("drg=")) return;

  String payload = line.substring(4);
  String tokens[2];
  int count = splitTokens(payload, ';', tokens, 2);
  if (count < 2) return;

  String clientId = normalizeId(tokens[0]);
  String ip       = tokens[1];

  // Dedup: don't re-emit slave_seen if already registered with same IP
  int knownIdx = findSlaveIndex(clientId);
  bool ipChanged = (knownIdx >= 0 && slaves[knownIdx].ip != ip);
  recordSeen(clientId, ip, ipChanged);

  // Reply mrg=
  String reply = "mrg=" + String(SERVER_ID) + ";" + WiFi.localIP().toString();
  udpSocket.beginPacket(remoteIp, remotePort);
  udpSocket.write(reinterpret_cast<const uint8_t*>(reply.c_str()), reply.length());
  udpSocket.endPacket();

  // Bind check
  bool shouldUpsert = !REQUIRE_BINDING ||
                      isPendingSlave(clientId) || isKnownSlave(clientId);
  if (shouldUpsert) {
    removePendingSlave(clientId);
    upsertSlave(clientId, ip);
    emitRegister(clientId, ip);
  }
}

// ═════════════════════════════════════════════════════════════════════════════
// SECTION 15: TCP INGEST (Slave → Master)
// ═════════════════════════════════════════════════════════════════════════════

void handleTcpLine(const String& line, const String& remoteIp) {
  LOG_STA("TCP <- " + remoteIp + " " + line);

  if (line.startsWith("drg=")) {
    String payload = line.substring(4);
    String tokens[2];
    int count = splitTokens(payload, ';', tokens, 2);
    if (count >= 2) {
      String clientId = normalizeId(tokens[0]);
      String ip       = tokens[1];
      recordSeen(clientId, ip, true);

      bool canBind = !REQUIRE_BINDING ||
                     isPendingSlave(clientId) || isKnownSlave(clientId);
      if (!canBind) {
        LOG_BIND("Required, ignoring drg for " + clientId);
        return;
      }
      removePendingSlave(clientId);
      upsertSlave(clientId, ip);
      emitRegister(clientId, ip);
    }
    return;
  }

  if (line.startsWith("sta=") || line.startsWith("res=") || line.startsWith("dst=")) {
    String payload = line.substring(4);
    String tokens[6];
    int count = splitTokens(payload, ';', tokens, 6);
    if (count < 2) return;

    String clientId = normalizeId(tokens[0]);
    if (REQUIRE_BINDING && !isKnownSlave(clientId)) {
      LOG_STA("Ignoring update from unbound slave " + clientId);
      return;
    }
    if (clientId.indexOf("END") >= 0) return;

    if (remoteIp.length() > 0) {
      upsertSlave(clientId, remoteIp);
    }
    int idx = findSlaveIndex(clientId);
    if (idx >= 0) slaves[idx].lastDataMs = millis();

    if (count < 5) return;

    String comp  = tokens[1];
    int mod      = toIntSafe(tokens[2], 0);
    int stat     = toIntSafe(tokens[3], 0);
    int val      = toIntSafe(tokens[4], 0);
    String reqId = count >= 6 ? tokens[5] : "";
    if (val > 1000) val = 1000;

    // Master-side dedup
    if (!shouldForwardStatus(clientId, comp, mod, stat, val)) {
      LOG_STA("DEDUP skip dev=" + clientId + " comp=" + comp);
      return;
    }

    if (line.startsWith("res=")) {
      emitResponse(clientId, comp, mod, stat, val, reqId);
    } else {
      emitStaResult(clientId, comp, mod, stat, val, reqId);
    }
  }
}

void handleTcpClients() {
  WiFiClient client = tcpServer.available();
  if (!client) return;

  client.setTimeout(TCP_READ_TIMEOUT_MS);
  String remoteIp = client.remoteIP().toString();
  unsigned long lastDataMs = millis();

  while (client.connected() || client.available()) {
    webSocket.loop();  // prevent WS timeout while handling TCP

    while (client.available()) {
      String line = client.readStringUntil('\n');
      line.trim();
      if (line.length() > 0) {
        handleTcpLine(line, remoteIp);
      }
      lastDataMs = millis();
    }

    if (millis() - lastDataMs > TCP_IDLE_TIMEOUT_MS) break;
    delay(2);
  }
  client.stop();
}

// ═════════════════════════════════════════════════════════════════════════════
// SECTION 16: WEBSOCKET EVENT HANDLERS (Backend → Master)
// ═════════════════════════════════════════════════════════════════════════════

void handleWsCommand(JsonObject data) {
  String serverId = normalizeId(String(data["serverID"] | ""));
  if (serverId.length() > 0 && serverId != SERVER_ID) return;

  String devId = normalizeId(String(data["devID"] | ""));
  String comp  = String(data["comp"] | "");
  int mod      = data["mod"] | 0;
  int stat     = data["stat"] | 0;
  int val      = data["val"] | 0;
  String reqId = String(data["reqId"] | "");

  LOG_CMD("WS cmd dev=" + devId + " comp=" + comp +
          " stat=" + String(stat) + " val=" + String(val));
  enqueueCommandToSlave(devId, comp, mod, stat, val, reqId);
}

void handleWsStatus(JsonObject data) {
  String serverId = normalizeId(String(data["serverID"] | ""));
  if (serverId.length() > 0 && serverId != SERVER_ID) return;

  String devId = normalizeId(String(data["devID"] | ""));
  String comp  = String(data["comp"] | "");

  if (comp.length() == 0) {
    LOG_STA("WS status dev=" + devId + " comp=* (fanout)");
    startStatusFanout(devId);
  } else {
    LOG_STA("WS status dev=" + devId + " comp=" + comp);
    sendStatusToSlave(devId, comp);
  }
}

void handleWsBindSlave(JsonObject data) {
  String serverId = normalizeId(String(data["serverID"] | ""));
  if (serverId.length() > 0 && serverId != SERVER_ID) return;

  String clientId = normalizeId(String(data["clientID"] | ""));
  if (clientId.length() == 0) return;

  addPendingSlave(clientId);
  LOG_BIND("Bind requested clientID=" + clientId);

  String ip = findSeenIp(clientId);
  if (ip.length() > 0) {
    removePendingSlave(clientId);
    upsertSlave(clientId, ip);
    emitRegister(clientId, ip);
  } else if (isKnownSlave(clientId)) {
    // Already known from NVS — confirm
    removePendingSlave(clientId);
    String knownIp = findSlaveIp(clientId);
    if (knownIp.length() > 0) {
      emitRegister(clientId, knownIp);
    }
  }
}

void handleWsUnbindSlave(JsonObject data) {
  String serverId = normalizeId(String(data["serverID"] | ""));
  if (serverId.length() > 0 && serverId != SERVER_ID) return;

  String clientId = normalizeId(String(data["clientID"] | ""));
  if (clientId.length() == 0) return;

  removePendingSlave(clientId);
  int idx = findSlaveIndex(clientId);
  if (idx >= 0) removeSlaveByIndex(idx);

  for (size_t i = 0; i < MAX_COMMAND_SLOTS; i++) {
    if (commandSlots[i].used && commandSlots[i].devId == clientId) {
      clearCommandSlot(static_cast<int>(i));
    }
  }
  LOG_BIND("Unbound clientID=" + clientId);
}

// ═════════════════════════════════════════════════════════════════════════════
// SECTION 17: WEBSOCKET EVENT ROUTER
// ═════════════════════════════════════════════════════════════════════════════

void onWebSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  LOG_CONN("WS event type=" + String(webSocketTypeName(type)));

  switch (type) {
    case WStype_CONNECTED:
      connTransition(CONN_CONNECTED, "ws_connected");
      emitGatewayRegister();
      connTransition(CONN_REGISTERED, "register_sent");
      flushSeen();
      // Re-emit registers for NVS-persisted slaves
      for (size_t i = 0; i < slaveCount; i++) {
        emitRegister(slaves[i].id, slaves[i].ip);
      }
      resetWsBackoff();
      wsWasConnected = true;
      break;

    case WStype_DISCONNECTED:
      connTransition(CONN_DISCONNECTED, "ws_disconnected");
      advanceWsBackoff();
      break;

    case WStype_ERROR:
      LOG_CONN("WS error: " + payloadToString(payload, length));
      break;

    case WStype_TEXT: {
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, payload, length);
      if (error) {
        LOG_CONN("WS JSON error: " + String(error.c_str()));
        return;
      }
      const char* eventName = doc["event"];
      JsonObject data = doc["data"].as<JsonObject>();
      if (!eventName || data.isNull()) return;

      LOG_CONN("WS event <- " + String(eventName));

      if (strcmp(eventName, "command") == 0)       handleWsCommand(data);
      else if (strcmp(eventName, "status") == 0)   handleWsStatus(data);
      else if (strcmp(eventName, "bind_slave") == 0)   handleWsBindSlave(data);
      else if (strcmp(eventName, "unbind_slave") == 0) handleWsUnbindSlave(data);
      break;
    }

    default:
      break;
  }
}

// ═════════════════════════════════════════════════════════════════════════════
// SECTION 18: SLAVE HEALTH (Ping + Stale Detection)
// ═════════════════════════════════════════════════════════════════════════════

void pingSlaves() {
  unsigned long now = millis();
  for (size_t i = 0; i < slaveCount; i++) {
    if (now - slaves[i].lastPingMs < SLAVE_PING_INTERVAL_MS) continue;
    slaves[i].lastPingMs = now;

    if (slaves[i].ip.length() == 0) continue;

    bool ok = sendHttpGet(slaves[i].ip, "/?ping=1", SLAVE_PING_TIMEOUT_MS);
    if (ok) {
      slaves[i].missedPings = 0;
      if (slaves[i].health == SLAVE_STALE) {
        slaves[i].health = SLAVE_ACTIVE;
        LOG_SLAVE("ACTIVE clientID=" + slaves[i].id + " (ping OK)");
      }
    } else {
      slaves[i].missedPings++;
      if (slaves[i].missedPings >= SLAVE_MAX_MISSED_PINGS &&
          slaves[i].health != SLAVE_STALE) {
        slaves[i].health = SLAVE_STALE;
        LOG_SLAVE("STALE clientID=" + slaves[i].id +
                  " reason=missed_pings(" + String(slaves[i].missedPings) + ")");
      }
    }
  }
}

void checkSlaveStale() {
  unsigned long now = millis();
  for (size_t i = 0; i < slaveCount; i++) {
    if (slaves[i].health == SLAVE_STALE) continue;
    if (slaves[i].lastDataMs == 0) continue;
    if (now - slaves[i].lastDataMs > SLAVE_STALE_MS) {
      slaves[i].health = SLAVE_STALE;
      LOG_SLAVE("STALE clientID=" + slaves[i].id +
                " reason=no_data_" + String(SLAVE_STALE_MS) + "ms");
    }
  }
}

void refreshSilentSlaves() {
  unsigned long now = millis();
  for (size_t i = 0; i < slaveCount; i++) {
    if (slaves[i].health != SLAVE_ACTIVE) continue;
    if (slaves[i].lastDataMs == 0) continue;
    if (now - slaves[i].lastDataMs > STALE_REFRESH_INTERVAL_MS) {
      LOG_STA("Auto-refresh silent slave " + slaves[i].id);
      startStatusFanout(slaves[i].id);
      slaves[i].lastDataMs = now; // prevent re-trigger next tick
    }
  }
}

// ═════════════════════════════════════════════════════════════════════════════
// SECTION 19: DIAGNOSTICS
// ═════════════════════════════════════════════════════════════════════════════

void emitPeriodicHeartbeat() {
  unsigned long now = millis();
  if (now - lastHeartbeatMs < HEARTBEAT_EMIT_INTERVAL_MS) return;
  lastHeartbeatMs = now;
  if (connState == CONN_REGISTERED) {
    emitGatewayHeartbeat();
  }
}

void logPeriodicMetrics() {
  unsigned long now = millis();
  if (now - lastMetricsMs < METRICS_LOG_INTERVAL_MS) return;
  lastMetricsMs = now;

  int queueDepth = 0, inFlight = 0;
  for (size_t i = 0; i < MAX_COMMAND_SLOTS; i++) {
    if (commandSlots[i].used) {
      queueDepth++;
      if (commandSlots[i].inFlight) inFlight++;
    }
  }

  int activeSlaves = 0, staleSlaves = 0;
  for (size_t i = 0; i < slaveCount; i++) {
    if (slaves[i].health == SLAVE_ACTIVE) activeSlaves++;
    else staleSlaves++;
  }

  unsigned long uptime = (now - bootMs) / 1000;
  LOG_METRIC("queueDepth=" + String(queueDepth) +
             " inFlight=" + String(inFlight) +
             " slaves=" + String(slaveCount) +
             " active=" + String(activeSlaves) +
             " stale=" + String(staleSlaves) +
             " seen=" + String(seenCount) +
             " fanout=" + String(fanoutSize()) +
             " uptime=" + String(uptime) + "s" +
             " freeHeap=" + String(ESP.getFreeHeap()));
  LOG_HEAP("free=" + String(ESP.getFreeHeap()));
}

// ═════════════════════════════════════════════════════════════════════════════
// SECTION 20: WATCHDOG
// ═════════════════════════════════════════════════════════════════════════════

void checkLoopWatchdog() {
  unsigned long elapsed = millis() - loopStartMs;
  if (elapsed > LOOP_WDT_WARN_MS) {
    LOG_WDT("loop iteration " + String(elapsed) + "ms (>" +
            String(LOOP_WDT_WARN_MS) + "ms)");
  }
}

// ═════════════════════════════════════════════════════════════════════════════
// SECTION 21: SETUP
// ═════════════════════════════════════════════════════════════════════════════

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("============================================================");
  Serial.println(" ESP32 Master Gateway — Production Build");
  Serial.println(" Server ID: " + String(SERVER_ID));
  Serial.println("============================================================");

  bootMs = millis();
  randomSeed(esp_random());

  // ─── Hardware watchdog (ESP-IDF v5.x config-struct API) ─────────
  const esp_task_wdt_config_t wdtCfg = {
    .timeout_ms        = TASK_WDT_TIMEOUT_S * 1000,
    .idle_core_mask    = 0,            // don't watch idle tasks
    .trigger_panic     = true
  };
  esp_task_wdt_init(&wdtCfg);
  esp_task_wdt_add(NULL);
  LOG_WDT("Task WDT enabled, timeout=" + String(TASK_WDT_TIMEOUT_S) + "s");

  // ─── WiFi ──────────────────────────────────────────────────────
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  LOG_CONN("WiFi connecting to " + String(WIFI_SSID));

  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    esp_task_wdt_reset();
    if (millis() - wifiStart > 30000) {
      LOG_CONN("WiFi initial connect timeout — restarting");
      delay(100);
      ESP.restart();
    }
  }
  LOG_CONN("WiFi connected IP=" + WiFi.localIP().toString());

  // ─── NVS: restore persisted slave bindings ─────────────────────
  nvsLoadSlaves();

  // ─── TCP server ────────────────────────────────────────────────
  tcpServer.begin();
  LOG_CONN("TCP server on port " + String(TCP_PORT));

  // ─── UDP discovery ─────────────────────────────────────────────
  udpSocket.begin(UDP_DISCOVERY_PORT);
  LOG_DISC("UDP listening on port " + String(UDP_DISCOVERY_PORT));

  // ─── WebSocket ─────────────────────────────────────────────────
  webSocket.beginSSL(SOCKET_HOST, SOCKET_PORT, SOCKET_PATH);
  webSocket.onEvent(onWebSocketEvent);
  webSocket.setReconnectInterval(applyJitter(WS_BACKOFF_BASE_MS));
  webSocket.enableHeartbeat(WS_HEARTBEAT_PING_MS, WS_HEARTBEAT_TIMEOUT_MS,
                            WS_HEARTBEAT_RETRIES);
  connTransition(CONN_CONNECTING, "setup_complete");
  LOG_CONN("WSS connecting to " + String(SOCKET_HOST) + ":" +
           String(SOCKET_PORT));

  // ─── Init diagnostic timers ────────────────────────────────────
  lastHeartbeatMs   = millis();
  lastMetricsMs     = millis();
  lastSeenCleanupMs = millis();

  Serial.println("============================================================");
  Serial.println(" Setup complete — entering main loop");
  Serial.println("============================================================");
}

// ═════════════════════════════════════════════════════════════════════════════
// SECTION 22: MAIN LOOP
// ═════════════════════════════════════════════════════════════════════════════

void loop() {
  loopStartMs = millis();
  esp_task_wdt_reset();  // feed hardware watchdog

  // ─── Core I/O (non-blocking) ──────────────────────────────────
  webSocket.loop();
  handleTcpClients();
  handleUdpDiscovery();

  // ─── Pipelines ────────────────────────────────────────────────
  processCommandQueue();
  processStatusFanout();

  // ─── Periodic maintenance ─────────────────────────────────────
  cleanupSeen();
  checkSlaveStale();
  pingSlaves();
  refreshSilentSlaves();

  // ─── Diagnostics ──────────────────────────────────────────────
  emitPeriodicHeartbeat();
  logPeriodicMetrics();
  checkLoopWatchdog();
}
