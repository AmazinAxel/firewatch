#include "airnow.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

namespace airnow {

void fetch(GovAqi& gov) {
  if (strlen(AIRNOW_TOKEN) == 0) return;

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.useHTTP10(true);  // no chunked encoding — we parse the raw stream
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.setConnectTimeout(HTTP_TIMEOUT_MS);
  // The current-generation observation endpoint (the classic zipCode API
  // only reported PM2.5 here; this one returns every pollutant, matching
  // the homelab webserver's AirNow number).
  if (!http.begin(client,
                  "https://www.airnowapi.org/aq/observation/current/"
                  "ziplatlong/?format=application/json&latitude=" LAT
                  "&longitude=" LON "&API_KEY=" AIRNOW_TOKEN))
    return;

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("airnow: HTTP %d\n", code);
    http.end();
    return;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, http.getStream());
  http.end();
  if (err || !doc.is<JsonArray>()) {
    Serial.println("airnow: bad json");
    return;
  }

  // One entry per monitor/pollutant (PM2.5, OZONE...); report the worst,
  // matching the AirNow site's headline number.
  int worst = -1;
  for (JsonObject o : doc.as<JsonArray>()) {
    int a = o["nowcastAQI"] | -1;
    if (a > worst) worst = a;
  }
  if (worst >= 0) {
    gov.aqi = worst;
    gov.ok = true;
  }
}

}  // namespace airnow
