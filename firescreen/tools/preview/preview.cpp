// Host-side dashboard preview: renders the same scenarios as
// scripts/fake_data.py through the real render/chart code and writes PGM
// images. Build & run with tools/preview/run.sh, which converts to PNG.
#include <Adafruit_GFX.h>

#include <cstdio>
#include <cstring>

#include "../../src/model.h"
#include "../../src/render.h"
#include "../../src/timeutil.h"

namespace {

// 2026-07-12 19:03:12 UTC == 12:03pm PDT (Sunday).
constexpr time_t kNow = 1783882992;

int16_t wave(int i, int n, int lo, int hi) {
  float s = 0.5f + 0.5f * sinf(i * 3.0f * static_cast<float>(M_PI) / n);
  return static_cast<int16_t>(lo + (hi - lo) * s);
}

void fillHistory(History& h, bool range7d, int lo, int hi) {
  h.ok = true;
  h.range7d = range7d;
  h.buckets = range7d ? CHART_BUCKETS_7D : CHART_BUCKETS_24H;
  h.endT = kNow;
  h.startT = kNow - (range7d ? 7 * 24 : 24) * 3600;
  for (int i = 0; i < h.buckets; ++i) h.v[i] = wave(i, h.buckets, lo, hi);
}

CurrentWeather cw(float t, float hi, float lo, int code, float wind, int deg,
                  float visMi = 10) {
  CurrentWeather w;
  w.ok = true;
  w.tempF = t;
  w.hiF = hi;
  w.loF = lo;
  w.wmoCode = code;
  w.windMph = wind;
  w.windDeg = deg;
  w.visMi = visMi;
  return w;
}

void days(Dashboard& d, int code1, int code2, float hi, float lo,
          int aqi1 = 42, int aqi2 = 137) {
  const char* names[] = {"Sat", "Sun"};
  int codes[] = {code1, code2};
  int aqis[] = {aqi1, aqi2};
  for (int i = 0; i < 2; ++i) {
    d.day[i].ok = true;
    strcpy(d.day[i].name, names[i]);
    d.day[i].wmoCode = codes[i];
    d.day[i].hiF = hi - i * 4;
    d.day[i].loF = lo - i * 3;
    d.day[i].windMph = 3 + i * 10;
    d.day[i].windDeg = 45 + i * 180;
    d.day[i].aqiMax = aqis[i];
  }
}

void aq(Dashboard& d, int a, int p25, int p10, int gov) {
  d.aq = {true, a, p25, p10};
  d.gov = {true, gov};
}

void savePgm(GFXcanvas1& c, const char* name) {
  char path[128];
  snprintf(path, sizeof(path), "%s.pgm", name);
  FILE* f = fopen(path, "w");
  fprintf(f, "P2\n%d %d\n255\n", SCREEN_W, SCREEN_H);
  for (int y = 0; y < SCREEN_H; ++y) {
    for (int x = 0; x < SCREEN_W; ++x)
      fprintf(f, "%d ", c.getPixel(x, y) ? 255 : 0);
    fprintf(f, "\n");
  }
  fclose(f);
  printf("wrote %s\n", path);
}

}  // namespace

int main() {
  timeutil::setupTz();
  GFXcanvas1 canvas(SCREEN_W, SCREEN_H);

  {  // normal
    Dashboard d;
    d.now = kNow;
    d.cur = cw(82, 90, 70, 0, 3, 45);
    days(d, 1, 3, 92, 67);
    aq(d, 34, 34, 12, 28);
    fillHistory(d.hist, false, 15, 55);
    d.batt = {true, 4.0f, 87};
    render::draw(canvas, d);
    savePgm(canvas, "preview_normal");
  }
  {  // hazardous: 3-digit everything, sub-mile visibility
    Dashboard d;
    d.now = kNow;
    d.cur = cw(102, 110, 68, 95, 23, 225, 0.8f);
    days(d, 95, 61, 108, 70, 412, 388);
    aq(d, 402, 402, 309, 311);
    fillHistory(d.hist, false, 250, 460);
    d.batt = {true, 3.4f, 12};
    render::draw(canvas, d);
    savePgm(canvas, "preview_hazardous");
  }
  {  // very unhealthy: longest label
    Dashboard d;
    d.now = kNow;
    d.cur = cw(95, 99, 71, 2, 8, 315);
    days(d, 2, 45, 97, 69);
    aq(d, 258, 258, 145, 201);
    fillHistory(d.hist, false, 120, 290);
    d.batt = {true, 3.8f, 54};
    render::draw(canvas, d);
    savePgm(canvas, "preview_very_unhealthy");
  }
  {  // freezing: negative temps + snow
    Dashboard d;
    d.now = kNow;
    d.cur = cw(-12, -5, -22, 73, 31, 0);
    days(d, 73, 71, -3, -18);
    aq(d, 51, 51, 22, 48);
    fillHistory(d.hist, false, 20, 80);
    d.batt = {true, 4.2f, 100};
    render::draw(canvas, d);
    savePgm(canvas, "preview_freezing");
  }
  {  // 7-day range with gaps
    Dashboard d;
    d.now = kNow;
    d.cur = cw(82, 90, 70, 0, 3, 45);
    days(d, 1, 3, 92, 67);
    aq(d, 77, 77, 41, 65);
    fillHistory(d.hist, true, 30, 180);
    for (int i = 20; i < 24; ++i) d.hist.v[i] = -1;
    d.batt = {true, 3.9f, 66};
    render::draw(canvas, d);
    savePgm(canvas, "preview_week");
  }
  {  // everything missing
    Dashboard d;
    render::draw(canvas, d);
    savePgm(canvas, "preview_missing");
  }
  {  // spinner overlay on the last frame
    render::drawSpinnerBox(canvas);
    savePgm(canvas, "preview_spinner");
  }
  return 0;
}
