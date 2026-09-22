#include "testmode.h"

#include <Arduino.h>
#include <ArduinoJson.h>

#include "config.h"
#include "display.h"
#include "model.h"
#include "render.h"

namespace testmode {
namespace {

void applyPayload(const JsonDocument& doc, Dashboard& d) {
  d = Dashboard{};  // absent sections stay ok=false and render as dashes

  JsonObjectConst w = doc["weather"];
  if (!w.isNull()) {
    d.cur.ok = true;
    d.cur.tempF = w["temp"] | 0.0f;
    d.cur.hiF = w["hi"] | 0.0f;
    d.cur.loF = w["lo"] | 0.0f;
    d.cur.wmoCode = w["code"] | 0;
    d.cur.isDay = w["isDay"] | true;
    d.cur.windMph = w["wind"] | 0.0f;
    d.cur.windDeg = w["windDir"] | 0;
    d.cur.visMi = w["vis"] | -1.0f;
    JsonArrayConst days = w["days"];
    for (int i = 0; i < 2 && i < static_cast<int>(days.size()); ++i) {
      JsonObjectConst o = days[i];
      d.day[i].ok = true;
      strncpy(d.day[i].name, o["name"] | "?", sizeof(d.day[i].name) - 1);
      d.day[i].wmoCode = o["code"] | 0;
      d.day[i].hiF = o["hi"] | 0.0f;
      d.day[i].loF = o["lo"] | 0.0f;
      d.day[i].windMph = o["wind"] | 0.0f;
      d.day[i].windDeg = o["windDir"] | 0;
      d.day[i].aqiMax = o["aqi"] | -1;
    }
  }

  JsonObjectConst aq = doc["aq"];
  if (!aq.isNull()) {
    d.aq.ok = true;
    d.aq.aqi = aq["aqi"] | 0;
    d.aq.pm25Aqi = aq["pm25"] | 0;
    d.aq.pm10Aqi = aq["pm10"] | 0;
  }
  if (doc["gov"].is<int>()) {
    d.gov.ok = true;
    d.gov.aqi = doc["gov"];
  }

  JsonObjectConst h = doc["history"];
  if (!h.isNull()) {
    d.hist.ok = true;
    d.hist.range7d = h["range7d"] | false;
    JsonArrayConst vals = h["values"];
    d.hist.buckets = min(static_cast<size_t>(CHART_MAX_BUCKETS), vals.size());
    for (uint16_t i = 0; i < d.hist.buckets; ++i)
      d.hist.v[i] = vals[i] | -1;
    d.hist.endT = doc["now"] | 0;
    if (d.hist.endT)
      d.hist.startT =
          d.hist.endT - (d.hist.range7d ? 7 * 24 : 24) * 3600;
  }

  if (doc["battery"].is<int>()) {
    d.batt.ok = true;
    d.batt.pct = doc["battery"];
    d.batt.volts = 3.3f + 0.9f * d.batt.pct / 100.0f;
  }
  d.now = doc["now"] | 0;
}

// Handle one input line. Returns false when the session should end.
bool handleLine(const String& line, Dashboard& d) {
  if (line == "exit" || line == "sleep") {
    Serial.println("testmode: leaving session");
    return false;
  }
  if (line == "full") {
    render::draw(display::canvas(), d);
    display::push(true);
    Serial.println("testmode: rendered (full)");
    return true;
  }
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, line);
  if (err) {
    Serial.printf("testmode: bad json (%s)\n", err.c_str());
    return true;
  }
  applyPayload(doc, d);
  render::draw(display::canvas(), d);
  display::push(!display::hasContent());
  Serial.println("testmode: rendered");
  return true;
}

// Read lines without blocking so a host disconnect (DTR drop) is noticed.
void session() {
  Serial.println(
      "testmode: send JSON per line to render; 'full' = full refresh; "
      "'exit'/'sleep' or closing the port ends the session");
  Dashboard d;
  String line;
  while (Serial) {
    while (Serial.available()) {
      char ch = static_cast<char>(Serial.read());
      if (ch == '\n') {
        line.trim();
        if (line.length() && !handleLine(line, d)) return;
        line = "";
      } else if (ch != '\r') {
        line += ch;
      }
    }
    delay(20);
  }
  Serial.println("testmode: host disconnected");
}

}  // namespace

void maybeRun() {
  uint32_t start = millis();
  while (millis() - start < SERIAL_TESTMODE_WINDOW_MS) {
    if (Serial.available()) {
      session();
      return;
    }
    delay(10);
  }
}

void interactiveWhileConnected() {
  if (!Serial) return;  // no USB host — sleep immediately (battery path)
  Serial.println("usb: monitor attached, staying awake until it closes");
  session();
}

}  // namespace testmode
