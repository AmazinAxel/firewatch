#include "buttons.h"

#include <Arduino.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>

#include "config.h"

namespace buttons {
namespace {

constexpr uint64_t kWakeMask = (1ULL << PIN_KEY_REFRESH) |
                               (1ULL << PIN_KEY_RANGE) |
                               (1ULL << PIN_KEY_FULL);

}  // namespace

Wake cause() {
  switch (esp_sleep_get_wakeup_cause()) {
    case ESP_SLEEP_WAKEUP_EXT1: {
      uint64_t status = esp_sleep_get_ext1_wakeup_status();
      if (status & (1ULL << PIN_KEY_RANGE)) return Wake::RangeKey;
      if (status & (1ULL << PIN_KEY_FULL)) return Wake::FullKey;
      return Wake::RefreshKey;
    }
    case ESP_SLEEP_WAKEUP_TIMER:
      return Wake::Timer;
    default:
      return Wake::FirstBoot;
  }
}

bool refreshKeyHeldLong() {
  pinMode(PIN_KEY_REFRESH, INPUT_PULLUP);
  uint32_t start = millis();
  while (digitalRead(PIN_KEY_REFRESH) == LOW) {
    if (millis() - start >= KEY_HOLD_FULL_MS) return true;
    delay(10);
  }
  return false;
}

void armAndSleep(uint64_t sleepUs) {
  // Buttons short to GND; keep their pullups alive through deep sleep.
  for (int pin : {PIN_KEY_REFRESH, PIN_KEY_RANGE, PIN_KEY_FULL}) {
    rtc_gpio_pullup_en(static_cast<gpio_num_t>(pin));
    rtc_gpio_pulldown_dis(static_cast<gpio_num_t>(pin));
  }
  esp_sleep_enable_ext1_wakeup(kWakeMask, ESP_EXT1_WAKEUP_ANY_LOW);
  esp_sleep_enable_timer_wakeup(sleepUs);
  esp_deep_sleep_start();
  __builtin_unreachable();
}

}  // namespace buttons
