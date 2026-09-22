// USB fake-data mode for exercising label/text sizing without real data.
//
// The device's native USB port disappears from the host during deep sleep,
// so there are two ways in, both driven by scripts/fake_data.py:
//  - maybeRun(): on a power-on/reset boot, any serial byte within a short
//    window starts a session immediately (before the normal fetch cycle).
//  - interactiveWhileConnected(): after every refresh cycle, if a host has
//    the port open (DTR asserted, e.g. fake_data.py or pio device monitor),
//    the device stays awake and keeps accepting payloads instead of
//    sleeping; it sleeps as soon as the port is closed.
// Timer/button wakes on battery see neither (no host attached), so the
// battery cost is zero. See fake_data.py for the payload schema.
#pragma once

namespace testmode {

// Boot-window entry. Returns after the window expires with no input, or
// when the session ends — the caller continues the normal refresh cycle.
void maybeRun();

// Post-cycle session while a USB host holds the port open. Returns when the
// host disconnects or sends "exit"/"sleep"; the caller then goes to sleep.
void interactiveWhileConnected();

}  // namespace testmode
