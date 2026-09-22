#include "chart.h"

#include <cstdio>
#include <cstring>
#include <ctime>

#include "config.h"
#include "render.h"
#include "timeutil.h"

// Font pointers exported by render.cpp (single owner of the generated tables).
namespace fonts {
extern const GFXfont *R18, *R28;
}

namespace chart {
namespace {

using render::INK;

constexpr int16_t GUTTER_B = 28;  // x-axis labels
constexpr int16_t TICK_LEN = 5;   // small label-to-axis connector lines
constexpr int16_t MIN_LABEL_GAP = 14;

// Smallest "nice" step giving at most ~4 intervals over `range`.
int niceStep(int range) {
  static const int kSteps[] = {5, 10, 20, 25, 50, 100, 200, 500};
  for (int s : kSteps)
    if (range <= s * 4) return s;
  return 500;
}

void thickLine(GFXcanvas1& c, int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
  c.drawLine(x0, y0, x1, y1, INK);
  c.drawLine(x0 + 1, y0, x1 + 1, y1, INK);
  c.drawLine(x0, y0 + 1, x1, y1 + 1, INK);
}

int16_t textWidth(GFXcanvas1& c, const char* s) {
  int16_t bx, by;
  uint16_t bw, bh;
  c.getTextBounds(s, 0, 0, &bx, &by, &bw, &bh);
  return static_cast<int16_t>(bw);
}

}  // namespace

void draw(GFXcanvas1& c, int16_t x, int16_t y, int16_t w, int16_t h,
          const History& hist) {
  // Data extent (only non-empty buckets).
  int vMin = 32767, vMax = -1;
  for (uint16_t i = 0; i < hist.buckets; ++i) {
    if (hist.v[i] < 0) continue;
    if (hist.v[i] < vMin) vMin = hist.v[i];
    if (hist.v[i] > vMax) vMax = hist.v[i];
  }
  if (!hist.ok || vMax < 0) {
    render::textFitted(c, {x, y, w, h}, "no data", {fonts::R28},
                       render::HAlign::Center);
    return;
  }

  // Y scale on nice bounds.
  int step = niceStep(vMax - vMin > 0 ? vMax - vMin : 1);
  int lo = vMin / step * step;
  int hi = (vMax + step - 1) / step * step;
  if (hi == lo) hi = lo + step;

  // Left gutter sized to the widest y label, so the plot claims the rest.
  char buf[8];
  snprintf(buf, sizeof(buf), "%d", hi);
  c.setFont(fonts::R18);
  const int16_t gutterL = textWidth(c, buf) + TICK_LEN + 6;
  const int16_t px = x + gutterL, py = y + 6;
  const int16_t pw = w - gutterL - 8, ph = h - 6 - GUTTER_B;
  const int16_t axisY = py + ph;

  auto mapY = [&](int v) {
    int32_t yy = axisY - static_cast<int32_t>(v - lo) * ph / (hi - lo);
    if (yy < py) yy = py;
    if (yy > axisY) yy = axisY;
    return static_cast<int16_t>(yy);
  };

  // Axes: solid left + bottom lines; y labels connect to the axis with the
  // same small tick marks the x axis uses.
  c.drawFastVLine(px, py, ph, INK);
  c.drawFastHLine(px, axisY, pw, INK);

  for (int v = lo; v <= hi; v += step) {
    int16_t gy = mapY(v);
    c.drawFastHLine(px - TICK_LEN, gy, TICK_LEN, INK);
    snprintf(buf, sizeof(buf), "%d", v);
    render::textFitted(
        c,
        {x, static_cast<int16_t>(gy - 10),
         static_cast<int16_t>(gutterL - TICK_LEN - 4), 20},
        buf, {fonts::R18}, render::HAlign::Right);
  }

  // X labels: "1pm"-style at 3-hour boundaries for 24h, weekday names at
  // local noon for 7d. Greedy placement skips any label that would collide
  // with the previously placed one.
  if (hist.startT > 0 && hist.endT > hist.startT) {
    auto timeToX = [&](time_t t) {
      return static_cast<int16_t>(
          px + static_cast<int64_t>(t - hist.startT) * pw /
                   (hist.endT - hist.startT));
    };
    c.setFont(fonts::R18);
    c.setTextColor(INK);
    int16_t lastRight = -1000;
    time_t t = (hist.startT / 3600 + 1) * 3600;  // next hour boundary
    for (; t < hist.endT; t += 3600) {
      struct tm tm;
      localtime_r(&t, &tm);
      if (hist.range7d) {
        if (tm.tm_hour != 12) continue;
        strncpy(buf, timeutil::weekdayName(tm.tm_wday), sizeof(buf));
      } else {
        if (tm.tm_hour % 3 != 0) continue;
        timeutil::fmtHour12(tm.tm_hour, buf);
      }
      int16_t lw = textWidth(c, buf);
      int16_t cxpos = timeToX(t);
      int16_t lx = cxpos - lw / 2;
      if (lx < px - 2) lx = px - 2;
      if (lx + lw > px + pw + 2) lx = px + pw + 2 - lw;
      if (lx < lastRight + MIN_LABEL_GAP) continue;  // would collide — skip
      c.drawFastVLine(cxpos, axisY, TICK_LEN, INK);
      c.setCursor(lx, axisY + 22);
      c.print(buf);
      lastRight = lx + lw;
    }
  }

  // Series: straight segments between neighbors, then clearly visible dots.
  int16_t prevX = -1, prevY = -1;
  for (uint16_t i = 0; i < hist.buckets; ++i) {
    if (hist.v[i] < 0) continue;
    int16_t sx = px + static_cast<int32_t>(i * 2 + 1) * pw / (hist.buckets * 2);
    int16_t sy = mapY(hist.v[i]);
    if (prevX >= 0) thickLine(c, prevX, prevY, sx, sy);
    prevX = sx;
    prevY = sy;
  }
  for (uint16_t i = 0; i < hist.buckets; ++i) {
    if (hist.v[i] < 0) continue;
    int16_t sx = px + static_cast<int32_t>(i * 2 + 1) * pw / (hist.buckets * 2);
    c.fillCircle(sx, mapY(hist.v[i]), 3, INK);
  }
}

}  // namespace chart
