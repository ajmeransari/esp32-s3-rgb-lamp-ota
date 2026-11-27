
/*
  ESP32-S3 RGB Lamp - WiFi Provisioning + RGB Controller

  Features:
  - WiFi setup portal hosted by ESP32
  - WiFi scanning
  - Credentials saved in Preferences (NVS)
  - Connection test before saving credentials
  - Recovery portal if WiFi connection fails
  - RGB web controller
  - RGB sliders, HEX display, presets, ON/OFF
  - Reset WiFi configuration
  - HTTPS OTA with manifest, SHA-256 verification, and status endpoints

  Required library:
  - Adafruit NeoPixel
  - ArduinoJson

  Built-in libraries:
  - WiFi.h
  - WebServer.h
  - Preferences.h
  - DNSServer.h

  NOTE:
  RGB_LED_PIN must match your board.
*/

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <DNSServer.h>
#include <Adafruit_NeoPixel.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <mbedtls/sha256.h>
#include <mbedtls/pk.h>
#include <mbedtls/base64.h>
#include <esp_ota_ops.h>

// ======================================================
// CONFIGURATION
// ======================================================

#define RGB_LED_PIN 48       // Change for your ESP32-S3 board
#define NUM_LEDS 1
#define LED_BRIGHTNESS 100

// Configure these before using OTA. The manifest and firmware must both be
// hosted on HTTPS endpoints trusted by this CA certificate.
#define FIRMWARE_VERSION "1.0.4"
const char* OTA_MANIFEST_URL = "https://raw.githubusercontent.com/ajmeransari/esp32-s3-rgb-lamp-ota/main/version.json";
const char OTA_ROOT_CA[] PROGMEM = R"CERT(
-----BEGIN CERTIFICATE-----
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBhMQswCQYDVQQG
EwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3d3cuZGlnaWNlcnQuY29tMSAw
HgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBHMjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUx
MjAwMDBaMGExCzAJBgNVBAYTAlVTMRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3
dy5kaWdpY2VydC5jb20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkq
hkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI2/Ou8jqJ
kTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx1x7e/dfgy5SDN67sH0NO
3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQq2EGnI/yuum06ZIya7XzV+hdG82MHauV
BJVJ8zUtluNJbd134/tJS7SsVQepj5WztCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyM
UNGPHgm+F6HmIcr9g+UQvIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQAB
o0IwQDAPBgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV5uNu
5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY1Yl9PMWLSn/pvtsr
F9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4NeF22d+mQrvHRAiGfzZ0JFrabA0U
WTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NGFdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBH
QRFXGU7Aj64GxJUTFy8bJZ918rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/
iyK5S9kJRaTepLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl
MrY=
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIICjzCCAhWgAwIBAgIQXIuZxVqUxdJxVt7NiYDMJjAKBggqhkjOPQQDAzCBiDELMAkGA1UEBhMC
VVMxEzARBgNVBAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0plcnNleSBDaXR5MR4wHAYDVQQKExVU
aGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNVBAMTJVVTRVJUcnVzdCBFQ0MgQ2VydGlmaWNhdGlv
biBBdXRob3JpdHkwHhcNMTAwMjAxMDAwMDAwWhcNMzgwMTE4MjM1OTU5WjCBiDELMAkGA1UEBhMC
VVMxEzARBgNVBAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0plcnNleSBDaXR5MR4wHAYDVQQKExVU
aGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNVBAMTJVVTRVJUcnVzdCBFQ0MgQ2VydGlmaWNhdGlv
biBBdXRob3JpdHkwdjAQBgcqhkjOPQIBBgUrgQQAIgNiAAQarFRaqfloI+d61SRvU8Za2EurxtW2
0eZzca7dnNYMYf3boIkDuAUU7FfO7l0/4iGzzvfUinngo4N+LZfQYcTxmdwlkWOrfzCjtHDix6Ez
nPO/LlxTsV+zfTJ/ijTjeXmjQjBAMB0GA1UdDgQWBBQ64QmG1M8ZwpZ2dEl23OA1xmNjmjAOBgNV
HQ8BAf8EBAMCAQYwDwYDVR0TAQH/BAUwAwEB/zAKBggqhkjOPQQDAwNoADBlAjA2Z6EWCNzklwBB
HU6+4WMBzzuqQhFkoJ2UOQIReVx7Hfpkue4WQrO/isIJxOzksU0CMQDpKmFHjFJKS04YcPbWRNZu
9YO6bVi9JNlWSOrvxKJGgYhqOkbRqZtNyWHa0V1Xahg=
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAwTzELMAkGA1UE
BhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2VhcmNoIEdyb3VwMRUwEwYDVQQD
EwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQG
EwJVUzEpMCcGA1UEChMgSW50ZXJuZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMT
DElTUkcgUm9vdCBYMTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54r
Vygch77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+0TM8ukj1
3Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6UA5/TR5d8mUgjU+g4rk8K
b4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sWT8KOEUt+zwvo/7V3LvSye0rgTBIlDHCN
Aymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyHB5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ
4Q7e2RCOFvu396j3x+UCB5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf
1b0SHzUvKBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWnOlFu
hjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTnjh8BCNAw1FtxNrQH
usEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbwqHyGO0aoSCqI3Haadr8faqU9GY/r
OPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CIrU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4G
A1UdDwEB/wQEAwIBBjAPBgNVHRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY
9umbbjANBgkqhkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ3BebYhtF8GaV
0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KKNFtY2PwByVS5uCbMiogziUwt
hDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJw
TdwJx4nLCgdNbOhdjsnvzqvHu7UrTkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nx
e5AW0wdeRlN8NwdCjNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZA
JzVcoyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq4RgqsahD
YVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPAmRGunUHBcnWEvgJBQl9n
JEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57demyPxgcYxn/eR44/KJ4EBs+lVDR3veyJ
m+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)CERT";
const size_t OTA_MAX_FIRMWARE_BYTES = 0x200000; // 2 MiB OTA app slot ceiling
// Public verification key only. The matching private key must stay in GitHub Actions secrets.
const char OTA_SIGNING_PUBLIC_KEY[] PROGMEM = "-----BEGIN PUBLIC KEY-----\nMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA3bjdJ9Ih9MpMiHNF/Gd5\nrTx29Aq+73NXDaxQGTXhMt3xmHRzOTmFgCq969d57zPC7PjNBAW+Qlker4zqIApI\nloz4yoK69BqKLeEpObjXeAT0nBrR+wt1fwmDDjcnc9bXdojZeDHSU9kvdgbrHLhD\nK8AVBG9fAos0/urnxhmRqCyHd1d6GRLyXfeKXJdMUwWT6yoma/6E0HQR8McWvhUc\nSdOEItGurGWPbCkNmCYDvsEsyhJfseG9UWh05MQTNVpGIkHwa4JbZ6nKAi9DywLM\n4FRXrOspHC1+hEoXVGkF7lL5hhHE5pqEOq9xGoXYOsPUBKrzKhNBIHBw+pcyo0N2\nGQIDAQAB\n-----END PUBLIC KEY-----";
const char* OTA_RELEASE_PREFIX = "https://github.com/ajmeransari/esp32-s3-rgb-lamp-ota/releases/download/";

const char* AP_SSID = "RGB-Lamp-Setup";
const char* AP_PASSWORD = "RGBsetup123";  // Prototype password

IPAddress apIP(192, 168, 4, 1);
IPAddress netMask(255, 255, 255, 0);

// ======================================================
// OBJECTS
// ======================================================

WebServer server(80);
DNSServer dnsServer;
Preferences preferences;

Adafruit_NeoPixel rgb(
  NUM_LEDS,
  RGB_LED_PIN,
  NEO_GRB + NEO_KHZ800
);

// ======================================================
// GLOBAL STATE
// ======================================================

String savedSSID = "";
String savedPassword = "";

String pendingSSID = "";
String pendingPassword = "";

int red = 0;
int green = 0;
int blue = 0;

bool ledEnabled = true;
bool provisioningMode = false;
String otaStatusMessage = "Not checked";
String otaAvailableVersion = "";
String otaFirmwareUrl = "";
String otaExpectedSha256 = "";
size_t otaExpectedSize = 0;
bool otaUpdateAvailable = false;
bool otaBusy = false;
int otaProgress = 0;

bool connectionAttemptActive = false;
bool connectionSucceeded = false;
bool connectionFailed = false;

unsigned long connectionStartTime = 0;
unsigned long rebootAt = 0;

const unsigned long WIFI_TIMEOUT = 20000;
const unsigned long REBOOT_DELAY = 2500;

// ======================================================
// RGB LED
// ======================================================

void updateLED() {
  if (ledEnabled) {
    rgb.setPixelColor(0, rgb.Color(red, green, blue));
  } else {
    rgb.setPixelColor(0, rgb.Color(0, 0, 0));
  }

  rgb.show();
}

// ======================================================
// HTML / JSON ESCAPING
// ======================================================

String htmlEscape(const String& input) {
  String output;
  output.reserve(input.length() + 16);

  for (int i = 0; i < input.length(); i++) {
    char c = input[i];

    switch (c) {
      case '&':  output += "&amp;";  break;
      case '<':  output += "&lt;";  break;
      case '>':  output += "&gt;";  break;
      case '"': output += "&quot;"; break;
      case '\'': output += "&#39;"; break;
      default:   output += c;
    }
  }

  return output;
}

String jsonEscape(const String& input) {
  String output;
  output.reserve(input.length() + 16);

  for (int i = 0; i < input.length(); i++) {
    char c = input[i];

    if (c == '"' || c == '\\') {
      output += '\\';
      output += c;
    } else if (c == '\n') {
      output += "\\n";
    } else if (c == '\r') {
      output += "\\r";
    } else if (c == '\t') {
      output += "\\t";
    } else if ((uint8_t)c < 0x20) {
      // Ignore unsupported control characters
    } else {
      output += c;
    }
  }

  return output;
}

bool isNumericVersion(const String& version) {
  int dots = 0; bool segmentHasDigit = false;
  for (size_t i = 0; i < version.length(); i++) {
    char c = version[i];
    if (c == '.') { if (!segmentHasDigit) return false; dots++; segmentHasDigit = false; }
    else if (c >= '0' && c <= '9') segmentHasDigit = true;
    else return false;
  }
  return dots == 2 && segmentHasDigit;
}

bool isSha256Hex(const char* value) {
  if (strlen(value) != 64) return false;
  for (size_t i = 0; i < 64; i++) {
    char c = value[i];
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) return false;
  }
  return true;
}

bool isTrustedOtaUrl(const char* url, const String& version) {
  String expected = String(OTA_RELEASE_PREFIX) + "v" + version + "/rgb-lamp-v" + version + ".bin";
  return String(url) == expected;
}

bool verifyManifestSignature(const char* version, const char* url, const char* sha256,
                             size_t imageSize, const char* signatureBase64) {
  String payload = String("ESP32-RGB-OTA-v1\nversion:") + version + "\nurl:" + url +
                   "\nsha256:" + sha256 + "\nsize:" + String((unsigned long)imageSize) + "\n";
  uint8_t signature[512]; size_t signatureLength = 0;
  if (mbedtls_base64_decode(signature, sizeof(signature), &signatureLength,
                            reinterpret_cast<const unsigned char*>(signatureBase64), strlen(signatureBase64)) != 0) return false;
  mbedtls_pk_context key; mbedtls_pk_init(&key);
  int rc = mbedtls_pk_parse_public_key(&key,
      reinterpret_cast<const unsigned char*>(OTA_SIGNING_PUBLIC_KEY), strlen(OTA_SIGNING_PUBLIC_KEY) + 1);
  if (rc == 0) {
    uint8_t digest[32];
    mbedtls_sha256(reinterpret_cast<const unsigned char*>(payload.c_str()), payload.length(), digest, 0);
    rc = mbedtls_pk_verify(&key, MBEDTLS_MD_SHA256, digest, sizeof(digest), signature, signatureLength);
  }
  mbedtls_pk_free(&key);
  return rc == 0;
}

bool versionIsNewer(const String& candidate, const String& current) {
  if (!isNumericVersion(candidate) || !isNumericVersion(current)) return false;
  unsigned long a[3] = {0, 0, 0};
  unsigned long b[3] = {0, 0, 0};
  String ca = candidate, cb = current;
  for (int i = 0; i < 3; i++) {
    int dot = ca.indexOf('.');
    String part = dot < 0 ? ca : ca.substring(0, dot);
    a[i] = strtoul(part.c_str(), nullptr, 10);
    ca = dot < 0 ? "" : ca.substring(dot + 1);
    dot = cb.indexOf('.');
    part = dot < 0 ? cb : cb.substring(0, dot);
    b[i] = strtoul(part.c_str(), nullptr, 10);
    cb = dot < 0 ? "" : cb.substring(dot + 1);
  }
  for (int i = 0; i < 3; i++) {
    if (a[i] > b[i]) return true;
    if (a[i] < b[i]) return false;
  }
  return false;
}

// ======================================================
// PREFERENCES / WIFI CREDENTIALS
// ======================================================

void loadCredentials() {
  preferences.begin("rgb-device", true);

  savedSSID = preferences.getString("ssid", "");
  savedPassword = preferences.getString("password", "");

  preferences.end();
}

void saveCredentials(const String& ssid, const String& password) {
  preferences.begin("rgb-device", false);

  preferences.putString("ssid", ssid);
  preferences.putString("password", password);

  preferences.end();

  savedSSID = ssid;
  savedPassword = password;
}

void clearCredentials() {
  preferences.begin("rgb-device", false);

  preferences.remove("ssid");
  preferences.remove("password");

  preferences.end();

  savedSSID = "";
  savedPassword = "";
}

bool setupRequested() {
  preferences.begin("rgb-device", true);
  bool requested = preferences.getBool("setup", false);
  preferences.end();

  return requested;
}

void clearSetupRequest() {
  preferences.begin("rgb-device", false);
  preferences.remove("setup");
  preferences.end();
}

void requestSetupMode() {
  preferences.begin("rgb-device", false);
  preferences.putBool("setup", true);
  preferences.end();
}

// ======================================================
// RGB CONTROL PAGE
// Existing controller interface retained
// ======================================================

const char rgbPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>RGB Lamp Controller</title>

<style>
body {
  font-family: Arial, sans-serif;
  background: #111827;
  color: white;
  text-align: center;
  margin: 0;
  padding: 20px;
}

.container {
  max-width: 420px;
  margin: auto;
  background: #1f2937;
  padding: 24px;
  border-radius: 20px;
}

.preview {
  width: 130px;
  height: 130px;
  margin: 20px auto;
  border-radius: 50%;
  background: rgb(0,0,0);
  transition: 0.15s;
}

.slider {
  width: 100%;
  margin: 12px 0;
}

.value {
  font-size: 18px;
  font-weight: bold;
}

button {
  border: none;
  border-radius: 10px;
  padding: 12px 18px;
  margin: 5px;
  font-size: 15px;
  cursor: pointer;
}

.presets {
  margin-top: 20px;
}

#hex {
  font-size: 22px;
  letter-spacing: 2px;
  margin: 12px;
}

a {
  color: #93c5fd;
}

.setup-link {
  display: inline-block;
  margin-top: 22px;
}
</style>
</head>

<body>

<div class="container">

<h2>RGB Lamp Controller</h2>

<div class="preview" id="preview"></div>

<div id="hex">#000000</div>

<label>Red: <span class="value" id="rv">0</span></label>
<input class="slider" type="range" min="0" max="255" value="0" id="r">

<label>Green: <span class="value" id="gv">0</span></label>
<input class="slider" type="range" min="0" max="255" value="0" id="g">

<label>Blue: <span class="value" id="bv">0</span></label>
<input class="slider" type="range" min="0" max="255" value="0" id="b">

<br><br>

<button onclick="toggleLED()" id="toggleBtn">Turn OFF</button>

<div class="presets">
<h3>Presets</h3>

<button onclick="setColor(255,0,0)">Red</button>
<button onclick="setColor(0,255,0)">Green</button>
<button onclick="setColor(0,0,255)">Blue</button>
<button onclick="setColor(255,255,255)">White</button>
<button onclick="setColor(255,100,0)">Orange</button>
<button onclick="setColor(255,0,255)">Magenta</button>
<button onclick="setColor(0,255,255)">Cyan</button>
<button onclick="setColor(255,255,0)">Yellow</button>
<button onclick="setColor(0,0,0)">Off</button>
</div>

<a class="setup-link" href="/setup">Wi-Fi settings</a>
<hr style="margin-top:24px;opacity:.25">
<h3>Firmware update</h3>
<p>Installed version: <strong id="fwVersion">loading</strong></p>
<p id="otaStatus">Loading update status...</p>
<button onclick="checkUpdate()" id="checkUpdateBtn">Check for update</button>
<button onclick="installUpdate()" id="installUpdateBtn" style="display:none">Install update</button>
<div style="background:#374151;border-radius:8px;height:10px;margin:12px 0"><div id="otaBar" style="height:10px;width:0;background:#22c55e;border-radius:8px"></div></div>

</div>

<script>
const r = document.getElementById("r");
const g = document.getElementById("g");
const b = document.getElementById("b");

let enabled = true;

function updateUI() {
  const R = Number(r.value);
  const G = Number(g.value);
  const B = Number(b.value);

  document.getElementById("rv").textContent = R;
  document.getElementById("gv").textContent = G;
  document.getElementById("bv").textContent = B;

  const hex = "#" +
    R.toString(16).padStart(2,"0") +
    G.toString(16).padStart(2,"0") +
    B.toString(16).padStart(2,"0");

  document.getElementById("hex").textContent = hex.toUpperCase();

  const preview = document.getElementById("preview");
  preview.style.background = enabled ? hex : "#000000";

  preview.style.boxShadow = enabled
    ? `0 0 35px ${hex}`
    : "none";

  sendColor();
}

function sendColor() {
  fetch(`/set?r=${r.value}&g=${g.value}&b=${b.value}`)
    .catch(err => console.log(err));
}

function setColor(R, G, B) {
  r.value = R;
  g.value = G;
  b.value = B;
  updateUI();
}

function toggleLED() {
  enabled = !enabled;

  document.getElementById("toggleBtn").textContent =
    enabled ? "Turn OFF" : "Turn ON";

  document.getElementById("preview").style.background =
    enabled ? `rgb(${r.value},${g.value},${b.value})` : "#000000";

  document.getElementById("preview").style.boxShadow =
    enabled ? `0 0 35px rgb(${r.value},${g.value},${b.value})` : "none";

  fetch(`/toggle?state=${enabled ? 1 : 0}`)
    .catch(err => console.log(err));
}

async function loadOtaStatus() {
  try {
    const result = await (await fetch("/ota/status")).json();
    document.getElementById("fwVersion").textContent = result.version;
    document.getElementById("otaStatus").textContent = result.supported
      ? result.message
      : "No OTA app slot is available. Select an OTA partition scheme and install it once over USB.";
    document.getElementById("checkUpdateBtn").disabled = !result.supported;
  } catch (error) { document.getElementById("otaStatus").textContent = "Update status unavailable."; }
}

async function checkUpdate() {
  const label = document.getElementById("otaStatus");
  label.textContent = "Checking for firmware updates...";
  try {
    const response = await fetch("/ota/check", {method:"POST"});
    const result = await response.json();
    label.textContent = result.message;
    document.getElementById("installUpdateBtn").style.display = result.updateAvailable ? "inline-block" : "none";
  } catch (error) { label.textContent = "Could not check for updates."; }
}

async function installUpdate() {
  document.getElementById("installUpdateBtn").disabled = true;
  document.getElementById("otaStatus").textContent = "Installing update. Keep the lamp powered; it will restart.";
  try {
    const response = await fetch("/ota/update", {method:"POST"});
    const result = await response.json();
    document.getElementById("otaStatus").textContent = result.message;
  } catch (error) { document.getElementById("otaStatus").textContent = "Update failed or the lamp is restarting."; }
}

r.addEventListener("input", updateUI);
g.addEventListener("input", updateUI);
b.addEventListener("input", updateUI);

updateUI();
loadOtaStatus();
</script>

</body>
</html>
)rawliteral";

// ======================================================
// WIFI SETUP PAGE
// ======================================================

const char setupHTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>RGB Lamp Setup</title>

<style>
body {
  font-family: Arial;
  background: #111827;
  color: white;
  margin: 0;
  padding: 20px;
}

.container {
  max-width: 430px;
  margin: auto;
  background: #1f2937;
  padding: 24px;
  border-radius: 18px;
}

input, select, button {
  width: 100%;
  box-sizing: border-box;
  padding: 13px;
  margin: 10px 0;
  border-radius: 9px;
  border: none;
  font-size: 16px;
}

button {
  background: #2563eb;
  color: white;
  cursor: pointer;
}

.secondary {
  background: #374151;
}

small {
  color: #cbd5e1;
}

#status {
  margin-top: 15px;
  padding: 10px;
  border-radius: 8px;
}

</style>
</head>

<body>
<div class="container">

<h2>RGB Lamp Setup</h2>
<p>Connect your lamp to your Wi-Fi network.</p>

<button class="secondary" onclick="scanNetworks()">Scan Wi-Fi Networks</button>

<select id="networks">
<option value="">Select Wi-Fi network</option>
</select>

<form id="wifiForm">
<input id="ssid" name="ssid" placeholder="Wi-Fi network name" required>
<input id="password" name="password" type="password" placeholder="Wi-Fi password">

<button type="submit" id="connectBtn">Connect</button>
</form>

<div id="status"></div>

<small>Your Wi-Fi credentials are stored on this device.</small>

<script>
const statusBox = document.getElementById("status");
const connectBtn = document.getElementById("connectBtn");

async function scanNetworks() {
  statusBox.textContent = "Scanning nearby networks...";

  const list = document.getElementById("networks");
  list.innerHTML = '<option value="">Scanning...</option>';

  try {
    const response = await fetch("/scan");
    if (!response.ok) throw new Error("Scan failed");

    const networks = await response.json();

    list.innerHTML = '<option value="">Select Wi-Fi network</option>';

    networks.forEach(network => {
      const option = document.createElement("option");
      option.value = network.ssid;
      option.textContent =
        network.ssid + " (" + network.rssi + " dBm)" +
        (network.secure ? " 🔒" : "");

      list.appendChild(option);
    });

    statusBox.textContent = networks.length
      ? "Select a network or enter its name manually."
      : "No networks found. Try scanning again.";
  } catch (error) {
    statusBox.textContent = "Wi-Fi scan failed. Try again.";
  }
}

document.getElementById("networks").addEventListener("change", function() {
  document.getElementById("ssid").value = this.value;
});

document.getElementById("wifiForm").addEventListener("submit", async function(e) {
  e.preventDefault();

  const ssid = document.getElementById("ssid").value.trim();
  const password = document.getElementById("password").value;

  if (!ssid) {
    statusBox.textContent = "Enter a Wi-Fi network name.";
    return;
  }

  connectBtn.disabled = true;
  connectBtn.textContent = "Connecting...";
  statusBox.textContent = "Testing Wi-Fi connection. Please wait.";

  try {
    const body = new URLSearchParams();
    body.append("ssid", ssid);
    body.append("password", password);

    const response = await fetch("/connect", {
      method: "POST",
      headers: {
        "Content-Type": "application/x-www-form-urlencoded"
      },
      body: body.toString()
    });

    const result = await response.json();

    if (!response.ok) {
      throw new Error(result.message || "Unable to start connection");
    }

    statusBox.textContent = "Connecting to " + ssid + "...";

    const started = Date.now();

    while (Date.now() - started < 25000) {
      await new Promise(resolve => setTimeout(resolve, 1000));

      const statusResponse = await fetch("/connection-status");
      const connection = await statusResponse.json();

      if (connection.status === "connected") {
        statusBox.textContent =
          "Connected successfully! Device is restarting...";

        setTimeout(() => {
          window.location.href = "http://" + connection.ip;
        }, 3000);

        return;
      }

      if (connection.status === "failed") {
        statusBox.textContent =
          "Connection failed. Check password or signal and try again.";

        connectBtn.disabled = false;
        connectBtn.textContent = "Connect";
        return;
      }
    }

    statusBox.textContent = "Connection timed out. Try again.";
    connectBtn.disabled = false;
    connectBtn.textContent = "Connect";

  } catch (error) {
    statusBox.textContent = "Error: " + error.message;
    connectBtn.disabled = false;
    connectBtn.textContent = "Connect";
  }
});
</script>

</div>
</body>
</html>
)rawliteral";

// ======================================================
// SETUP PORTAL
// ======================================================

void startProvisioning() {
  provisioningMode = true;

  connectionAttemptActive = false;
  connectionSucceeded = false;
  connectionFailed = false;

  Serial.println("\nStarting provisioning mode");

  WiFi.disconnect();
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(apIP, apIP, netMask);

  if (WiFi.softAP(AP_SSID, AP_PASSWORD)) {
    Serial.println("Setup AP started");
    Serial.print("SSID: ");
    Serial.println(AP_SSID);
    Serial.print("Setup IP: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Failed to start AP");
  }

  dnsServer.start(53, "*", apIP);

  // Setup page
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", setupHTML);
  });

  server.on("/setup", HTTP_GET, []() {
    server.send(200, "text/html", setupHTML);
  });

  // WiFi scan endpoint
  server.on("/scan", HTTP_GET, []() {
    int count = WiFi.scanNetworks(false, true);

    String json = "[";
    bool first = true;

    for (int i = 0; i < count; i++) {
      String ssid = WiFi.SSID(i);
      if (ssid.length() == 0) continue;

      if (!first) json += ",";
      first = false;

      json += "{";
      json += "\"ssid\":\"" + jsonEscape(ssid) + "\",";
      json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
      json += "\"secure\":" +
        String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false");
      json += "}";
    }

    json += "]";

    WiFi.scanDelete();

    server.send(200, "application/json", json);
  });

  // Begin connection attempt
  server.on("/connect", HTTP_POST, []() {
    if (connectionAttemptActive) {
      server.send(409, "application/json",
        "{\"message\":\"Connection already in progress\"}");
      return;
    }

    String newSSID = server.arg("ssid");
    String newPassword = server.arg("password");

    newSSID.trim();

    if (newSSID.length() == 0) {
      server.send(400, "application/json",
        "{\"message\":\"SSID is required\"}");
      return;
    }

    pendingSSID = newSSID;
    pendingPassword = newPassword;

    connectionAttemptActive = true;
    connectionSucceeded = false;
    connectionFailed = false;

    connectionStartTime = millis();

    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(pendingSSID.c_str(), pendingPassword.c_str());

    Serial.print("Connecting to: ");
    Serial.println(pendingSSID);

    server.send(200, "application/json",
      "{\"status\":\"connecting\"}");
  });

  // Connection status endpoint
  server.on("/connection-status", HTTP_GET, []() {
    if (connectionSucceeded) {
      String json = "{\"status\":\"connected\",\"ip\":\"";
      json += WiFi.localIP().toString();
      json += "\"}";

      server.send(200, "application/json", json);
    } else if (connectionFailed) {
      server.send(200, "application/json",
        "{\"status\":\"failed\"}");
    } else if (connectionAttemptActive) {
      server.send(200, "application/json",
        "{\"status\":\"connecting\"}");
    } else {
      server.send(200, "application/json",
        "{\"status\":\"idle\"}");
    }
  });

  // Captive portal detection endpoints
  server.on("/generate_204", HTTP_GET, []() {
    server.send(200, "text/html", setupHTML);
  });

  server.on("/hotspot-detect.html", HTTP_GET, []() {
    server.send(200, "text/html", setupHTML);
  });

  server.on("/fwlink", HTTP_GET, []() {
    server.send(200, "text/html", setupHTML);
  });

  server.onNotFound([]() {
    server.sendHeader("Location", "http://192.168.4.1/");
    server.send(302, "text/plain", "");
  });

  server.begin();

  Serial.println("Setup web server started");
}

// ======================================================
// NORMAL MODE WEB SERVER
// ======================================================

void startNormalWebServer() {
  provisioningMode = false;

  // Main RGB controller
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", rgbPage);
  });

  // RGB control
  server.on("/set", HTTP_GET, []() {
    if (server.hasArg("r"))
      red = constrain(server.arg("r").toInt(), 0, 255);

    if (server.hasArg("g"))
      green = constrain(server.arg("g").toInt(), 0, 255);

    if (server.hasArg("b"))
      blue = constrain(server.arg("b").toInt(), 0, 255);

    updateLED();

    server.send(200, "text/plain", "OK");
  });

  // LED toggle
  server.on("/toggle", HTTP_GET, []() {
    if (server.hasArg("state")) {
      ledEnabled = server.arg("state").toInt() == 1;
    }

    updateLED();
    server.send(200, "text/plain", "OK");
  });

  // OTA state endpoint: also reports whether the selected partition table
  // provides a second application slot for safe updates.
  server.on("/ota/status", HTTP_GET, []() {
    const bool supported = esp_ota_get_next_update_partition(nullptr) != nullptr;
    String json = "{\"version\":\"" FIRMWARE_VERSION "\",\"supported\":";
    json += supported ? "true" : "false";
    json += ",\"busy\":" + String(otaBusy ? "true" : "false");
    json += ",\"progress\":" + String(otaProgress);
    json += ",\"updateAvailable\":" + String(otaUpdateAvailable ? "true" : "false");
    json += ",\"message\":\"" + jsonEscape(otaStatusMessage) + "\"}";
    server.send(200, "application/json", json);
  });

  // Manifest format: {"version":"1.0.2","url":"https://.../firmware.bin",
  // "sha256":"64 lowercase/uppercase hex digits","size":123456}
  server.on("/ota/check", HTTP_POST, []() {
    if (!esp_ota_get_next_update_partition(nullptr)) {
      otaUpdateAvailable = false;
      otaStatusMessage = "No OTA app slot is present. Select an OTA partition scheme and flash it once by USB.";
      server.send(409, "application/json", "{\"updateAvailable\":false,\"message\":\"" + jsonEscape(otaStatusMessage) + "\"}");
      return;
    }
    if (!WiFi.isConnected() || !OTA_MANIFEST_URL[0] || strstr(OTA_ROOT_CA, "PASTE_THE_CA") != nullptr) {
      otaUpdateAvailable = false;
      otaStatusMessage = "OTA needs Wi-Fi plus an HTTPS manifest URL and trusted CA certificate in the sketch.";
      server.send(503, "application/json", "{\"updateAvailable\":false,\"message\":\"" + jsonEscape(otaStatusMessage) + "\"}");
      return;
    }
    WiFiClientSecure tls;
    tls.setCACert(OTA_ROOT_CA);
    HTTPClient http;
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    if (!http.begin(tls, OTA_MANIFEST_URL)) {
      otaStatusMessage = "Could not open the HTTPS update manifest.";
      server.send(502, "application/json", "{\"updateAvailable\":false,\"message\":\"" + jsonEscape(otaStatusMessage) + "\"}"); return;
    }
    int code = http.GET();
    if (code != HTTP_CODE_OK) {
      http.end(); otaStatusMessage = "Update server returned HTTP " + String(code) + ".";
      server.send(502, "application/json", "{\"updateAvailable\":false,\"message\":\"" + jsonEscape(otaStatusMessage) + "\"}"); return;
    }
    DynamicJsonDocument manifest(2048);
    DeserializationError error = deserializeJson(manifest, http.getStream());
    http.end();
    const char* version = manifest["version"] | "";
    const char* url = manifest["url"] | "";
    const char* sha256 = manifest["sha256"] | "";
    const char* signature = manifest["signature"] | "";
    size_t imageSize = manifest["size"] | 0;
    if (error || strlen(version) == 0 ||
        !isSha256Hex(sha256) || !isNumericVersion(version) || imageSize == 0 || imageSize > OTA_MAX_FIRMWARE_BYTES) {
      otaUpdateAvailable = false;
      otaStatusMessage = "The update manifest is invalid or incomplete.";
      server.send(502, "application/json", "{\"updateAvailable\":false,\"message\":\"" + otaStatusMessage + "\"}"); return;
    }
    if (!isTrustedOtaUrl(url, String(version)) || !verifyManifestSignature(version, url, sha256, imageSize, signature)) {
      otaUpdateAvailable = false;
      otaStatusMessage = "Manifest signature or trusted release URL validation failed.";
      server.send(403, "application/json", "{\"updateAvailable\":false,\"message\":\"" + jsonEscape(otaStatusMessage) + "\"}"); return;
    }
    otaAvailableVersion = version;
    otaFirmwareUrl = url;
    otaExpectedSha256 = sha256;
    otaExpectedSize = imageSize;
    otaUpdateAvailable = versionIsNewer(otaAvailableVersion, FIRMWARE_VERSION);
    otaStatusMessage = otaUpdateAvailable ? "Version " + otaAvailableVersion + " is ready to install." : "Firmware is up to date.";
    server.send(200, "application/json", "{\"updateAvailable\":" + String(otaUpdateAvailable ? "true" : "false") + ",\"message\":\"" + jsonEscape(otaStatusMessage) + "\"}");
  });

  server.on("/ota/update", HTTP_POST, []() {
    if (otaBusy) { server.send(409, "application/json", "{\"message\":\"An update is already running.\"}"); return; }
    if (!otaUpdateAvailable || otaFirmwareUrl.length() == 0 || otaExpectedSize == 0) {
      server.send(409, "application/json", "{\"message\":\"Check for a newer update first.\"}"); return;
    }
    otaBusy = true; otaProgress = 0; otaStatusMessage = "Downloading and verifying firmware...";
    WiFiClientSecure tls; tls.setCACert(OTA_ROOT_CA);
    HTTPClient http; http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    if (!http.begin(tls, otaFirmwareUrl)) {
      otaStatusMessage = "Could not open the firmware download."; otaBusy = false;
      server.send(502, "application/json", "{\"message\":\"" + jsonEscape(otaStatusMessage) + "\"}"); return;
    }
    int code = http.GET();
    int contentLength = http.getSize();
    if (code != HTTP_CODE_OK || contentLength <= 0 || (size_t)contentLength != otaExpectedSize || !Update.begin(otaExpectedSize)) {
      http.end(); otaStatusMessage = "Firmware size is wrong or the OTA slot is too small."; otaBusy = false;
      server.send(502, "application/json", "{\"message\":\"" + jsonEscape(otaStatusMessage) + "\"}"); return;
    }
    mbedtls_sha256_context shaCtx; mbedtls_sha256_init(&shaCtx); mbedtls_sha256_starts(&shaCtx, 0);
    WiFiClient* stream = http.getStreamPtr();
    uint8_t buffer[1024]; size_t received = 0; unsigned long lastData = millis();
    while (received < otaExpectedSize && millis() - lastData < 30000) {
      size_t available = stream->available();
      if (available == 0) { delay(2); continue; }
      size_t amount = stream->readBytes(reinterpret_cast<char*>(buffer), min(available, sizeof(buffer)));
      if (amount == 0 || Update.write(buffer, amount) != amount) break;
      mbedtls_sha256_update(&shaCtx, buffer, amount);
      received += amount; lastData = millis();
      otaProgress = (int)((received * 100) / otaExpectedSize);
    }
    uint8_t digest[32]; mbedtls_sha256_finish(&shaCtx, digest); mbedtls_sha256_free(&shaCtx); http.end();
    char actualSha[65];
    for (int i = 0; i < 32; i++) sprintf(actualSha + (i * 2), "%02x", digest[i]);
    actualSha[64] = '\0';
    if (received != otaExpectedSize || strcasecmp(actualSha, otaExpectedSha256.c_str()) != 0) {
      Update.abort(); otaStatusMessage = "SHA-256 did not match; the existing firmware remains active."; otaBusy = false;
      server.send(502, "application/json", "{\"message\":\"" + jsonEscape(otaStatusMessage) + "\"}"); return;
    }
    if (!Update.end(true)) {
      otaStatusMessage = "ESP32 could not finalize the verified firmware."; otaBusy = false;
      server.send(500, "application/json", "{\"message\":\"" + jsonEscape(otaStatusMessage) + "\"}"); return;
    }
    otaProgress = 100; otaStatusMessage = "Update verified. Restarting now.";
    server.send(200, "application/json", "{\"message\":\"" + jsonEscape(otaStatusMessage) + "\"}");
    delay(500); ESP.restart();
  });

  // Enter setup mode
  server.on("/setup", HTTP_GET, []() {
    server.send(200, "text/html",
      "<h2>Entering Wi-Fi setup mode...</h2>"
      "<p>Reconnect to RGB-Lamp-Setup after restart.</p>");

    requestSetupMode();
    rebootAt = millis() + 500;
  });

  // Clear Wi-Fi and force provisioning
  server.on("/resetwifi", HTTP_GET, []() {
    server.send(200, "text/html",
      "<h2>Wi-Fi configuration reset</h2>"
      "<p>Restarting device into setup mode...</p>");

    clearCredentials();
    requestSetupMode();
    rebootAt = millis() + 500;
  });

  server.onNotFound([]() {
    server.send(404, "text/plain", "Not found");
  });

  server.begin();

  Serial.println("RGB controller web server started");
}

// ======================================================
// CONNECT TO SAVED WIFI
// ======================================================

bool connectToSavedWiFi() {
  if (savedSSID.length() == 0) {
    return false;
  }

  Serial.print("Connecting to saved WiFi: ");
  Serial.println(savedSSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(savedSSID.c_str(), savedPassword.c_str());

  unsigned long startTime = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - startTime < 15000) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    return true;
  }

  Serial.println("Saved WiFi connection failed.");

  WiFi.disconnect();
  return false;
}

// ======================================================
// SETUP
// ======================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("================================");
  Serial.println("ESP32-S3 RGB Lamp Firmware");
  Serial.println("================================");

  rgb.begin();
  rgb.setBrightness(LED_BRIGHTNESS);
  rgb.clear();
  rgb.show();

  loadCredentials();

  bool forcedSetup = setupRequested();
  clearSetupRequest();

  if (!forcedSetup && savedSSID.length() > 0) {
    if (connectToSavedWiFi()) {
      startNormalWebServer();
    } else {
      startProvisioning();
    }
  } else {
    startProvisioning();
  }
}

// ======================================================
// LOOP
// ======================================================

void loop() {
  server.handleClient();

  if (provisioningMode) {
    dnsServer.processNextRequest();

    // Monitor pending WiFi connection
    if (connectionAttemptActive) {
      if (WiFi.status() == WL_CONNECTED) {
        connectionAttemptActive = false;
        connectionSucceeded = true;
        connectionFailed = false;

        saveCredentials(pendingSSID, pendingPassword);

        Serial.println("WiFi credentials saved.");
        Serial.print("Connected IP: ");
        Serial.println(WiFi.localIP());

        rebootAt = millis() + REBOOT_DELAY;
      }

      // Timeout
      else if (millis() - connectionStartTime > WIFI_TIMEOUT) {
        connectionAttemptActive = false;
        connectionSucceeded = false;
        connectionFailed = true;

        WiFi.disconnect();

        Serial.println("WiFi connection timed out.");
      }
    }
  }

  // Reboot after success or when setup/reset is requested
  if (rebootAt != 0 && millis() >= rebootAt) {
    rebootAt = 0;
    ESP.restart();
  }
}
