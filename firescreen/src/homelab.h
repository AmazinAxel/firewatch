// Homelab air-quality fetch: streams /getdata (24h) or /getdataFrom7Days
// through the PmStreamParser into chart buckets and derives the current
// AQI values from the newest sample.
#pragma once

#include "model.h"

namespace homelab {

// On success fills `hist` (bucketed chart data) and `aq` (latest AQIs).
// The response's Date header is used as the clock source: it fills `nowOut`
// when unset. Falls back to the newest sample timestamp so bucketing works
// even without the header.
void fetch(bool range7d, History& hist, AirQuality& aq, time_t& nowOut);

}  // namespace homelab
