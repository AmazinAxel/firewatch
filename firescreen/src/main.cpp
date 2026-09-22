// Firescreen — weather + air-quality dashboard for the TRMNL OG DIY Kit.
//
// Lifecycle: the device lives in deep sleep and wakes every
// REFRESH_INTERVAL_S (or on a button), shows a spinner at the cross
// intersection, fetches homelab AQI + Open-Meteo + AirNow, renders, updates
// the e-ink (fast partial refresh; full every PARTIAL_PER_FULL cycles) and
// goes back to sleep with WiFi and the panel powered down. Every data
// source is optional per cycle — failures render as dashes.

#include <Arduino.h>
#include <esp_adc_cal.h>

#include "airnow.h"
#include "buttons.h"
#include "config.h"
#include "display.h"
#include "homelab.h"
#include "model.h"
#include "net.h"
#include "render.h"
#include "testmode.h"
#include "timeutil.h"
#include "weather.h"

namespace {

RTC_DATA_ATTR uint32_t sRefreshCount = 0;
RTC_DATA_ATTR bool sRange7d = false;

Battery readBattery() {
  Battery b;
  pinMode(PIN_BATT_EN, OUTPUT);
  digitalWrite(PIN_BATT_EN, HIGH);  // power the voltage divider
  delay(10);
  uint32_t mv = 0;
  for (int i = 0; i < 8; ++i) mv += analogReadMilliVolts(PIN_BATT_ADC);
  digitalWrite(PIN_BATT_EN, LOW);
  b.volts = mv / 8 * 2 / 1000.0f;  // 1:2 divider
  if (b.volts < 2.0f) return b;    // no battery / divider unpowered
  float pct = (b.volts - 3.3f) / (4.15f - 3.3f) * 100.0f;
  b.pct = constrain(static_cast<int>(pct), 0, 100);
  b.ok = true;
  return b;
}

}  // namespace

void setup() {
  uint32_t wakeStart = millis();
  Serial.begin(115200);
  timeutil::setupTz();

  buttons::Wake wake = buttons::cause();
  bool forceFull = wake == buttons::Wake::FullKey;
  if (wake == buttons::Wake::RangeKey) sRange7d = !sRange7d;
  if (wake == buttons::Wake::RefreshKey && buttons::refreshKeyHeldLong())
    forceFull = true;

  bool restored = display::begin();
  Serial.printf("wake=%d range7d=%d restored=%d\n", static_cast<int>(wake),
                sRange7d, restored);

  if (wake == buttons::Wake::FirstBoot) testmode::maybeRun();

  if (display::hasContent()) display::showSpinner();

  Dashboard d;
  if (net::connect()) {
    net::rememberConnection();
    homelab::fetch(sRange7d, d.hist, d.aq, d.now);
    weather::fetch(d.cur, d.day, d.now);
    airnow::fetch(d.gov);
  } else {
    Serial.println("wifi: connect failed");
  }
  net::off();  // radio off before the (slow) e-ink update

  d.hist.range7d = sRange7d;
  d.batt = readBattery();

  render::draw(display::canvas(), d);
  ++sRefreshCount;
  bool full = forceFull || !display::hasContent() ||
              sRefreshCount % PARTIAL_PER_FULL == 0;
  display::push(full);

  // If a USB host is watching (monitor/fake_data.py), stay awake for it —
  // the native USB port vanishes from the host once we deep sleep.
  testmode::interactiveWhileConnected();

  display::finish();

  uint64_t elapsedUs = static_cast<uint64_t>(millis() - wakeStart) * 1000;
  uint64_t intervalUs = static_cast<uint64_t>(REFRESH_INTERVAL_S) * 1000000;
  uint64_t sleepUs =
      elapsedUs < intervalUs ? intervalUs - elapsedUs : 60 * 1000000ULL;
  Serial.printf("cycle %u (%s) took %ums; sleeping %llus\n",
                static_cast<unsigned>(sRefreshCount), full ? "full" : "partial",
                static_cast<unsigned>(millis() - wakeStart),
                sleepUs / 1000000);
  Serial.flush();
  buttons::armAndSleep(sleepUs);
}

void loop() {}  // never reached; setup() ends in deep sleep
