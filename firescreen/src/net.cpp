#include "net.h"

#include <ESPmDNS.h>
#include <WiFi.h>
#include <esp_wifi.h>

#include <cstring>

#include "config.h"

namespace net {
namespace {

// Survives deep sleep; cleared on power loss or wifi failure.
RTC_DATA_ATTR bool sCacheValid = false;
RTC_DATA_ATTR uint8_t sChannel = 0;
RTC_DATA_ATTR uint8_t sBssid[6] = {};
RTC_DATA_ATTR uint32_t sHomelabIp = 0;

bool waitConnected(uint32_t timeoutMs) {
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > timeoutMs) return false;
    delay(20);
  }
  return true;
}

}  // namespace

bool connect() {
  WiFi.persistent(false);  // don't wear flash with credential writes
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);  // full speed while awake; radio is cut in off()

  if (sCacheValid) {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD, sChannel, sBssid, true);
    if (waitConnected(WIFI_FAST_CONNECT_TIMEOUT_MS)) return true;
    // AP moved channel or roamed — fall back to a full scan.
    sCacheValid = false;
    WiFi.disconnect();
  }

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  if (!waitConnected(WIFI_CONNECT_TIMEOUT_MS)) return false;
  return true;
}

void rememberConnection() {
  if (WiFi.status() != WL_CONNECTED) return;
  sChannel = WiFi.channel();
  std::memcpy(sBssid, WiFi.BSSID(), 6);
  sCacheValid = true;
}

IPAddress homelabIp() {
  if (sHomelabIp != 0) return IPAddress(sHomelabIp);

  const char* host = HOMELAB_HOST;
  IPAddress ip;
  const char* dotLocal = std::strstr(host, ".local");
  if (dotLocal) {
    char name[64];
    size_t n = dotLocal - host;
    if (n >= sizeof(name)) return IPAddress();
    std::memcpy(name, host, n);
    name[n] = '\0';
    if (MDNS.begin("firescreen")) {
      ip = MDNS.queryHost(name, MDNS_QUERY_TIMEOUT_MS);
      MDNS.end();
    }
  } else {
    WiFi.hostByName(host, ip);
  }
  if (ip != IPAddress()) sHomelabIp = static_cast<uint32_t>(ip);
  return ip;
}

void invalidateHomelabIp() { sHomelabIp = 0; }

void off() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  esp_wifi_stop();
}

}  // namespace net
