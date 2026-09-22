// Open-Meteo fetch: current conditions + 3-day daily forecast for the
// configured LAT/LON, in Fahrenheit/mph. Free API, no key required.
#pragma once

#include "model.h"

namespace weather {

// Fills `cur` (current + today's H/L) and `days[0..1]` (next two days).
// On failure the ok flags stay false. If `nowOut` is unset (0) and the
// response carries a usable timestamp, it is filled as a clock fallback.
void fetch(CurrentWeather& cur, DayForecast days[2], time_t& nowOut);

// Map an Open-Meteo WMO weather code to an icon.
WeatherIcon iconFor(int wmoCode, bool isDay);

}  // namespace weather
