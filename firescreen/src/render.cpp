#include "render.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>

#include "aqi.h"
#include "chart.h"
#include "config.h"
#include "generated/fonts.h"
#include "generated/icons.h"
#include "timeutil.h"
#include "weather.h"

// The generated font/icon tables are static, so exactly one translation unit
// may include them; other modules access fonts through these pointers.
namespace fonts {
const GFXfont* R14 = &IosevkaR14;
const GFXfont* R18 = &IosevkaR18;
const GFXfont* R28 = &IosevkaR28;
const GFXfont* B28 = &IosevkaB28;
const GFXfont* B36 = &IosevkaB36;
const GFXfont* B44 = &IosevkaB44;
const GFXfont* B64 = &IosevkaB64;
const GFXfont* B80 = &IosevkaB80;
const GFXfont* B112 = &IosevkaB112;
const GFXfont* B144 = &IosevkaB144;
}  // namespace fonts

namespace render {
namespace {

void drawIcon(GFXcanvas1& c, const IconBmp& ic, int16_t x, int16_t y) {
  c.drawBitmap(x, y, ic.data, ic.w, ic.h, INK);
}

// Wind arrows point where the wind blows TO; the API reports FROM.
const IconBmp& arrowFor(const IconBmp table[8], int deg) {
  int to = (deg + 180) % 360;
  return table[((to + 22) / 45) % 8];
}

void fmtTemp(char* out, size_t n, float f) {
  snprintf(out, n, "%d%c", static_cast<int>(lroundf(f)), DEG_CH);
}

void fmtWind(char* out, size_t n, float mph) {
  snprintf(out, n, "%dmph", static_cast<int>(lroundf(mph)));
}

// --- top-left quadrant: current weather + two forecast days ----------------

void drawWeather(GFXcanvas1& c, const Dashboard& d) {
  const CurrentWeather& w = d.cur;
  char buf[24];

  // Current conditions. The temp's cap band with B80 in this box spans
  // y=27..85 (center 56); the 64px icon and the icon-over-text stacks
  // (wind, visibility) are all centered on that band.
  if (w.ok) {
    drawIcon(c, ICON_WEATHER_64[weather::iconFor(w.wmoCode, w.isDay)], 16, 24);
    fmtTemp(buf, sizeof(buf), w.tempF);
    textFitted(c, {92, 4, 200, 104}, buf, {fonts::B80, fonts::B64},
               HAlign::Left);

    char t[12];
    fmtTemp(t, sizeof(t), w.hiF);
    snprintf(buf, sizeof(buf), "H:%s", t);
    textFitted(c, {300, 16, 96, 36}, buf, {fonts::B28}, HAlign::Left);
    fmtTemp(t, sizeof(t), w.loF);
    snprintf(buf, sizeof(buf), "L:%s", t);
    textFitted(c, {300, 60, 96, 36}, buf, {fonts::B28}, HAlign::Left);

    // Wind: arrow stacked over the speed, both centered on x=444.
    drawIcon(c, arrowFor(ICON_ARROW_28, w.windDeg), 430, 24);
    fmtWind(buf, sizeof(buf), w.windMph);
    textFitted(c, {394, 60, 100, 36}, buf, {fonts::R28}, HAlign::Center);

    // Visibility: eye stacked over miles, centered on x=535.
    if (w.visMi >= 0) {
      drawIcon(c, ICON_EYE, 521, 24);
      if (w.visMi < 9.95f)
        snprintf(buf, sizeof(buf), "%.1fmi", w.visMi);
      else
        snprintf(buf, sizeof(buf), "%.0fmi", w.visMi);
      textFitted(c, {496, 60, 78, 36}, buf, {fonts::R28}, HAlign::Center);
    }
  } else {
    textFitted(c, {16, 4, 380, 104}, "-", {fonts::B80}, HAlign::Left);
  }

  // Forecast rows, spread across the quadrant width. Text boxes are
  // y+14 h34: the R28/B28 cap band spans y+21..y+41 (center y+31), so
  // 28px icons sit at y+17 and 20px arrows at y+21.
  for (int r = 0; r < 2; ++r) {
    const DayForecast& f = d.day[r];
    int16_t y = static_cast<int16_t>(120 + r * 62);
    int16_t ty = static_cast<int16_t>(y + 14);
    if (!f.ok) {
      textFitted(c, {16, ty, 380, 34}, "-", {fonts::R28}, HAlign::Left);
      continue;
    }
    textFitted(c, {16, ty, 60, 34}, f.name, {fonts::R28}, HAlign::Left);
    drawIcon(c, ICON_WEATHER_28[weather::iconFor(f.wmoCode, true)], 100,
             y + 17);
    fmtTemp(buf, sizeof(buf), f.hiF);
    textFitted(c, {140, ty, 76, 34}, buf, {fonts::B28}, HAlign::Left);
    drawIcon(c, ICON_WEATHER_28[ICO_MOON], 248, y + 17);
    fmtTemp(buf, sizeof(buf), f.loF);
    textFitted(c, {286, ty, 76, 34}, buf, {fonts::R28}, HAlign::Left);
    drawIcon(c, arrowFor(ICON_ARROW_20, f.windDeg), 388, y + 21);
    fmtWind(buf, sizeof(buf), f.windMph);
    textFitted(c, {416, ty, 80, 34}, buf, {fonts::R28}, HAlign::Left);
    if (f.aqiMax >= 0) {
      snprintf(buf, sizeof(buf), "%d", f.aqiMax);
      textFitted(c, {500, ty, 42, 34}, buf, {fonts::B28}, HAlign::Right);
      // Box tuned so the R18 label shares the value's baseline (ty+27).
      textFitted(c, {548, static_cast<int16_t>(ty + 8), 28, 26}, "aqi",
                 {fonts::R18}, HAlign::Left);
    }
  }
}

// --- right column: giant AQI + per-source rows ------------------------------

void drawAqiPanel(GFXcanvas1& c, const Dashboard& d) {
  char buf[16];
  if (d.aq.ok)
    snprintf(buf, sizeof(buf), "%d", d.aq.aqi);
  else
    strcpy(buf, "-");
  textFitted(c, {596, 4, 198, 160}, buf, {fonts::B144, fonts::B112},
             HAlign::Center);
  textFitted(c, {594, 174, 202, 60}, aqi::label(d.aq.ok ? d.aq.aqi : -1),
             {fonts::B44, fonts::B36, fonts::B28}, HAlign::Center);
}

void drawPmPanel(GFXcanvas1& c, const Dashboard& d) {
  struct Row {
    bool ok;
    int value;
    const char* label;
  } rows[] = {
      {d.aq.ok, d.aq.pm25Aqi, "pm2.5"},
      {d.aq.ok, d.aq.pm10Aqi, "pm10"},
      {d.gov.ok, d.gov.aqi, "gov"},
  };
  char buf[16];
  for (int i = 0; i < 3; ++i) {
    int16_t y = static_cast<int16_t>(260 + i * 60);
    if (rows[i].ok)
      snprintf(buf, sizeof(buf), "%d", rows[i].value);
    else
      strcpy(buf, "-");
    textFitted(c, {598, y, 100, 50}, buf, {fonts::B44, fonts::B36},
               HAlign::Right);
    textFitted(c, {712, static_cast<int16_t>(y + 7), 84, 36}, rows[i].label,
               {fonts::R28}, HAlign::Left);
  }
}

void drawStamp(GFXcanvas1& c, const Dashboard& d) {
  char clock[16] = "-";
  if (d.now != 0) {
    struct tm tm;
    localtime_r(&d.now, &tm);
    int h = tm.tm_hour % 12;
    if (h == 0) h = 12;
    snprintf(clock, sizeof(clock), "%d:%02d%s", h, tm.tm_min,
             tm.tm_hour < 12 ? "am" : "pm");
  }
  char buf[40];
  if (SHOW_BATTERY && d.batt.ok)
    snprintf(buf, sizeof(buf), "%s  %d%%", clock, d.batt.pct);
  else
    snprintf(buf, sizeof(buf), "%s", clock);
  textFitted(c, {620, 460, 177, 18}, buf, {fonts::R14}, HAlign::Right);
}

void drawCross(GFXcanvas1& c) {
  c.drawFastVLine(CROSS_X - 1, 0, SCREEN_H, INK);
  c.drawFastVLine(CROSS_X, 0, SCREEN_H, INK);
  c.drawFastHLine(0, CROSS_Y - 1, SCREEN_W, INK);
  c.drawFastHLine(0, CROSS_Y, SCREEN_W, INK);
}

}  // namespace

void textFitted(GFXcanvas1& c, const TextBox& box, const char* s,
                std::initializer_list<const GFXfont*> ladder, HAlign align) {
  if (!s || !*s) return;
  char buf[48];
  strncpy(buf, s, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';

  const GFXfont* chosen = nullptr;
  int16_t bx, by;
  uint16_t bw = 0, bh = 0;
  for (const GFXfont* f : ladder) {
    c.setFont(f);
    c.getTextBounds(buf, 0, 0, &bx, &by, &bw, &bh);
    chosen = f;
    if (static_cast<int16_t>(bw) <= box.w && static_cast<int16_t>(bh) <= box.h)
      break;
  }
  // Last resort: drop trailing characters until the width fits.
  while (strlen(buf) > 1 && static_cast<int16_t>(bw) > box.w) {
    buf[strlen(buf) - 1] = '\0';
    c.getTextBounds(buf, 0, 0, &bx, &by, &bw, &bh);
  }

  c.setFont(chosen);
  c.setTextColor(INK);
  int16_t cx = box.x - bx;
  if (align == HAlign::Center) cx = box.x + (box.w - bw) / 2 - bx;
  if (align == HAlign::Right) cx = box.x + box.w - bw - bx;
  // Baseline-based vertical centering: center the font's cap band (measured
  // from "0") rather than the string's own bounds, so strings with and
  // without descenders ("3mph" vs "H:90") share a baseline in equal boxes.
  int16_t rx, ry;
  uint16_t rw, capH;
  c.getTextBounds("0", 0, 0, &rx, &ry, &rw, &capH);
  int16_t cy = box.y + (box.h + static_cast<int16_t>(capH)) / 2;
  c.setCursor(cx, cy);
  c.print(buf);
}

void draw(GFXcanvas1& c, const Dashboard& d) {
  c.fillScreen(PAPER);
  drawWeather(c, d);
  chart::draw(c, 8, CROSS_Y + 8, CROSS_X - 18, SCREEN_H - CROSS_Y - 14,
              d.hist);
  drawAqiPanel(c, d);
  drawPmPanel(c, d);
  drawStamp(c, d);
  drawCross(c);
}

void drawSpinnerBox(GFXcanvas1& c) {
  int16_t bx = CROSS_X - SPINNER_BOX / 2;
  int16_t by = CROSS_Y - SPINNER_BOX / 2;
  c.fillRect(bx, by, SPINNER_BOX, SPINNER_BOX, PAPER);
  c.drawRoundRect(bx + 2, by + 2, SPINNER_BOX - 4, SPINNER_BOX - 4, 6, INK);
  c.drawRoundRect(bx + 3, by + 3, SPINNER_BOX - 6, SPINNER_BOX - 6, 6, INK);
  drawIcon(c, ICON_REFRESH, CROSS_X - ICON_REFRESH.w / 2,
           CROSS_Y - ICON_REFRESH.h / 2);
}

}  // namespace render
