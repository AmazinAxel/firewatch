// Firescreen central configuration: hardware pins, refresh cadence, endpoints
// and layout geometry. Secrets (WiFi, AirNow token) and location overrides
// arrive as -D defines from scripts/load_env.py reading the gitignored .env.
#pragma once

#include <cstdint>

// ---------------------------------------------------------------------------
// Hardware — Seeed XIAO ePaper Driver Board (TRMNL 7.5" OG DIY Kit).
// Pin map verified against Seeed's ESPHome example for this kit.
// ---------------------------------------------------------------------------
constexpr int PIN_EPD_SCK = 7;
constexpr int PIN_EPD_MOSI = 9;
constexpr int PIN_EPD_CS = 44;
constexpr int PIN_EPD_DC = 10;
constexpr int PIN_EPD_RST = 38;
constexpr int PIN_EPD_BUSY = 4;  // active-low (handled by the GxEPD2 driver)

// Buttons are active-low (pressed = GND) and need internal pullups. All
// three wake the device from deep sleep via one EXT1 any-low mask.
constexpr int PIN_KEY_REFRESH = 2;  // KEY1: refresh now; hold ~1s = full refresh
constexpr int PIN_KEY_RANGE = 3;    // KEY2: toggle 24h <-> 7d history chart
constexpr int PIN_KEY_FULL = 5;     // KEY3: refresh with full (ghost-clearing) update

constexpr int PIN_BATT_ADC = 1;  // battery voltage through a 1:2 divider
constexpr int PIN_BATT_EN = 6;   // must be driven HIGH to power the divider

// ---------------------------------------------------------------------------
// Behavior
// ---------------------------------------------------------------------------
constexpr uint32_t REFRESH_INTERVAL_S = 5 * 60;  // wake + redraw cadence
constexpr uint32_t PARTIAL_PER_FULL = 5;         // every Nth refresh is a full one
constexpr uint32_t KEY_HOLD_FULL_MS = 800;       // hold refresh key this long = full
// Boot window to enter USB test mode. Long enough for the host to
// re-enumerate the native USB port after a reset and send the knock byte.
// Only applies to power-on/reset boots, so it costs no battery.
constexpr uint32_t SERIAL_TESTMODE_WINDOW_MS = 4000;

constexpr uint32_t HTTP_TIMEOUT_MS = 8000;
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 10000;
constexpr uint32_t WIFI_FAST_CONNECT_TIMEOUT_MS = 4000;  // cached channel/BSSID path
constexpr uint32_t MDNS_QUERY_TIMEOUT_MS = 4000;

// POSIX TZ rule for America/Los_Angeles (chart labels, "updated" stamp).
#define TZ_POSIX "PST8PDT,M3.2.0,M11.1.0"

// ---------------------------------------------------------------------------
// Data sources (override via .env)
// ---------------------------------------------------------------------------
#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
#endif
#ifndef AIRNOW_TOKEN
#define AIRNOW_TOKEN ""
#endif
#ifndef HOMELAB_HOST
#define HOMELAB_HOST "alechomelab.local"
#endif
#ifndef ZIP
#define ZIP "98226"
#endif
#ifndef LAT
#define LAT "48.7877"
#endif
#ifndef LON
#define LON "-122.4432"
#endif

// ---------------------------------------------------------------------------
// Display / layout (800x480, 1-bit). The screen is split into four quadrants
// by a cross; the refresh spinner lives in a box at the intersection.
// ---------------------------------------------------------------------------
constexpr int16_t SCREEN_W = 800;
constexpr int16_t SCREEN_H = 480;
constexpr int16_t CROSS_X = 590;  // vertical divider (wide left for the chart)
constexpr int16_t CROSS_Y = 248;  // horizontal divider
constexpr int16_t SPINNER_BOX = 56;  // white-padded box for the refresh icon

// History chart downsampling: few enough points that each renders as a
// clearly visible dot, per the mockup.
constexpr uint16_t CHART_BUCKETS_24H = 44;  // one dot per ~33 min
constexpr uint16_t CHART_BUCKETS_7D = 52;   // one dot per ~3.2 h
constexpr uint16_t CHART_MAX_BUCKETS = 52;

constexpr bool SHOW_BATTERY = true;  // small % next to the "updated" stamp
