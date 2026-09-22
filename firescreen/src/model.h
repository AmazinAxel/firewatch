// Plain data model shared between the fetchers, the renderer and the USB
// test mode. Every section carries its own `ok` flag: a failed fetch leaves
// ok=false and the renderer shows dashes for that section instead of
// aborting the refresh.
#pragma once

#include <cstdint>
#include <ctime>

#include "config.h"

// Icon identity for a weather condition (indexes the generated icon tables).
enum WeatherIcon : uint8_t {
  ICO_SUN = 0,
  ICO_MOON,
  ICO_PARTLY_DAY,
  ICO_PARTLY_NIGHT,
  ICO_CLOUD,
  ICO_FOG,
  ICO_DRIZZLE,
  ICO_RAIN,
  ICO_SNOW,
  ICO_THUNDER,
  WEATHER_ICON_COUNT,
};

struct CurrentWeather {
  bool ok = false;
  float tempF = 0;
  float hiF = 0, loF = 0;  // today's forecast high/low
  float windMph = 0;
  int16_t windDeg = 0;  // meteorological: direction the wind blows FROM
  int16_t wmoCode = 0;
  bool isDay = true;
  float visMi = -1;  // current visibility in miles; <0 = unknown
};

struct DayForecast {
  bool ok = false;
  char name[4] = "";  // "Sat"
  float hiF = 0, loF = 0;
  float windMph = 0;
  int16_t windDeg = 0;
  int16_t wmoCode = 0;
  int16_t aqiMax = -1;  // forecast day-max US AQI; <0 = unknown
};

struct AirQuality {
  bool ok = false;
  int aqi = 0;      // headline: max(pm25Aqi, pm10Aqi) of the latest sample
  int pm25Aqi = 0;
  int pm10Aqi = 0;
};

struct GovAqi {  // AirNow observation for the configured ZIP
  bool ok = false;
  int aqi = 0;
};

// Downsampled AQI history. v[i] is the max AQI within bucket i covering
// [startT + i*bucketSec, startT + (i+1)*bucketSec); -1 marks an empty bucket.
struct History {
  bool ok = false;
  bool range7d = false;
  uint16_t buckets = 0;
  time_t startT = 0, endT = 0;
  int16_t v[CHART_MAX_BUCKETS] = {};
};

struct Battery {
  bool ok = false;
  float volts = 0;
  int pct = 0;
};

struct Dashboard {
  CurrentWeather cur;
  DayForecast day[2];  // next two days
  AirQuality aq;
  GovAqi gov;
  History hist;
  Battery batt;
  time_t now = 0;  // epoch; 0 = unknown (renders "--:--" stamp)
};
