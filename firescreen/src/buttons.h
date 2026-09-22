// Deep-sleep button wakeup: all three keys share one EXT1 mask (the S3
// supports "any pin low" wakeup) and the triggering pin is decoded from the
// EXT1 wakeup status on boot.
#pragma once

#include <cstdint>

namespace buttons {

enum class Wake {
  FirstBoot,   // power-on / reset / reflash
  Timer,       // periodic refresh
  RefreshKey,  // KEY1: refresh now (hold = full refresh)
  RangeKey,    // KEY2: toggle 24h <-> 7d history chart
  FullKey,     // KEY3: refresh with a full, ghost-clearing update
};

Wake cause();

// After a RefreshKey wake: true if the key is still held KEY_HOLD_FULL_MS
// later — the alternate "force a full refresh" gesture.
bool refreshKeyHeldLong();

// Arm timer + button wake sources and enter deep sleep. Never returns.
[[noreturn]] void armAndSleep(uint64_t sleepUs);

}  // namespace buttons
