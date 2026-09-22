// AQI history line chart: auto-scaled y axis with dotted gridlines, hour
// ("1pm") or weekday x labels with collision-avoiding placement, and the
// series drawn as visible dots connected by straight 2px segments.
#pragma once

#include <Adafruit_GFX.h>

#include "model.h"

namespace chart {

void draw(GFXcanvas1& c, int16_t x, int16_t y, int16_t w, int16_t h,
          const History& hist);

}  // namespace chart
