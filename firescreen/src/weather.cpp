#include "weather.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include "timeutil.h"

namespace weather {
namespace {

// Current + 3 daily entries (today for H/L, then the two forecast rows),
// already in Fahrenheit/mph and local (Pacific) dates.
constexpr const char* kUrl =
    "https://api.open-meteo.com/v1/forecast"
    "?latitude=" LAT "&longitude=" LON
    "&current=temperature_2m,weather_code,wind_speed_10m,wind_direction_10m,"
    "is_day,visibility"
    "&daily=weather_code,temperature_2m_max,temperature_2m_min,"
    "wind_speed_10m_max,wind_direction_10m_dominant"
    "&temperature_unit=fahrenheit&wind_speed_unit=mph"
    "&timezone=America%2FLos_Angeles&forecast_days=3";

// Open-Meteo's separate air-quality service: hourly US AQI forecast, no key.
constexpr const char* kAqiUrl =
    "https://air-quality-api.open-meteo.com/v1/air-quality"
    "?latitude=" LAT "&longitude=" LON
    "&hourly=us_aqi&forecast_days=3&timezone=America%2FLos_Angeles";

// Fill each forecast day's max US AQI by matching hourly entries to the
// day's local date ("YYYY-MM-DD" prefix of the ISO timestamps).
void fetchAqiForecast(WiFiClientSecure& client, DayForecast days[2],
                      const char dates[2][11]) {
  HTTPClient http;
  http.useHTTP10(true);
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.setConnectTimeout(HTTP_TIMEOUT_MS);
  if (!http.begin(client, kAqiUrl)) return;
  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("aqi-forecast: HTTP %d\n", code);
    http.end();
    return;
  }
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, http.getStream());
  http.end();
  if (err) {
    Serial.printf("aqi-forecast: json %s\n", err.c_str());
    return;
  }
  JsonArrayConst times = doc["hourly"]["time"];
  JsonArrayConst values = doc["hourly"]["us_aqi"];
  for (size_t i = 0; i < times.size() && i < values.size(); ++i) {
    const char* t = times[i] | "";
    int v = values[i] | -1;
    if (v < 0) continue;
    for (int di = 0; di < 2; ++di)
      if (strncmp(t, dates[di], 10) == 0 && v > days[di].aqiMax)
        days[di].aqiMax = static_cast<int16_t>(v);
  }
}

}  // namespace

void fetch(CurrentWeather& cur, DayForecast days[2], time_t& nowOut) {
  WiFiClientSecure client;
  client.setInsecure();  // public weather data; skip cert store to save RAM/time
  HTTPClient http;
  http.useHTTP10(true);  // no chunked encoding — we parse the raw stream
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.setConnectTimeout(HTTP_TIMEOUT_MS);
  const char* dateHeader[] = {"Date"};
  http.collectHeaders(dateHeader, 1);
  if (!http.begin(client, kUrl)) return;

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("weather: HTTP %d\n", code);
    http.end();
    return;
  }
  if (nowOut == 0 && http.hasHeader("Date"))
    nowOut = timeutil::parseHttpDate(http.header("Date").c_str());

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, http.getStream());
  http.end();
  if (err) {
    Serial.printf("weather: json %s\n", err.c_str());
    return;
  }

  JsonObject c = doc["current"];
  JsonObject daily = doc["daily"];
  if (c.isNull() || daily.isNull()) return;

  cur.tempF = c["temperature_2m"] | 0.0f;
  cur.wmoCode = c["weather_code"] | 0;
  cur.isDay = (c["is_day"] | 1) != 0;
  cur.windMph = c["wind_speed_10m"] | 0.0f;
  cur.windDeg = c["wind_direction_10m"] | 0;
  float visM = c["visibility"] | -1.0f;
  cur.visMi = visM >= 0 ? visM / 1609.34f : -1;
  cur.hiF = daily["temperature_2m_max"][0] | 0.0f;
  cur.loF = daily["temperature_2m_min"][0] | 0.0f;
  cur.ok = true;

  char dates[2][11] = {};
  for (int i = 0; i < 2; ++i) {
    DayForecast& d = days[i];
    int idx = i + 1;  // daily[0] is today
    const char* date = daily["time"][idx] | "";
    strncpy(dates[i], date, sizeof(dates[i]) - 1);
    int y, m, dd;
    if (sscanf(date, "%d-%d-%d", &y, &m, &dd) == 3)
      strncpy(d.name, timeutil::weekdayName(timeutil::weekdayFromYmd(y, m, dd)),
              sizeof(d.name));
    d.wmoCode = daily["weather_code"][idx] | 0;
    d.hiF = daily["temperature_2m_max"][idx] | 0.0f;
    d.loF = daily["temperature_2m_min"][idx] | 0.0f;
    d.windMph = daily["wind_speed_10m_max"][idx] | 0.0f;
    d.windDeg = daily["wind_direction_10m_dominant"][idx] | 0;
    d.ok = true;
  }

  fetchAqiForecast(client, days, dates);
}

}  // namespace weather
