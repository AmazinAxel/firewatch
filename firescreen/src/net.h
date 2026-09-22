// WiFi lifecycle tuned for battery: fast reconnect using the channel/BSSID
// cached in RTC memory from the previous wake (skips the AP scan, typically
// halving connect time), mDNS resolution of the homelab's .local hostname
// with the resulting IP cached in RTC memory, and a hard radio-off.
#pragma once

#include <IPAddress.h>

namespace net {

// Connect to WIFI_SSID. Returns false on timeout (dashboard still renders).
bool connect();

// Resolve HOMELAB_HOST. Uses the RTC-cached IP when present; falls back to
// an mDNS query for *.local names or DNS otherwise. Returns INADDR_NONE on
// failure. Call invalidateHomelabIp() if a request to the cached IP fails.
IPAddress homelabIp();
void invalidateHomelabIp();

// Cache the current connection's channel/BSSID for the next wake.
void rememberConnection();

// Disconnect and power the radio down completely.
void off();

}  // namespace net
