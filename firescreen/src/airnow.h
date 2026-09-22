// AirNow ("gov") AQI for the configured ZIP. Requires AIRNOW_TOKEN in .env;
// without it the fetch is skipped and the row renders a dash.
#pragma once

#include "model.h"

namespace airnow {

void fetch(GovAqi& gov);

}  // namespace airnow
