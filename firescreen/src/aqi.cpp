#include "aqi.h"

#include <cmath>

namespace aqi {
namespace {

struct Breakpoint {
  float cLow, cHigh;
  int iLow, iHigh;
};

// EPA breakpoint tables (same values as the homelab webserver page).
constexpr Breakpoint kPm25[] = {
    {0.0f, 12.0f, 0, 50},      {12.1f, 35.4f, 51, 100},
    {35.5f, 55.4f, 101, 150},  {55.5f, 150.4f, 151, 200},
    {150.5f, 250.4f, 201, 300}, {250.5f, 500.4f, 301, 500},
};
constexpr Breakpoint kPm10[] = {
    {0, 54, 0, 50},       {55, 154, 51, 100},
    {155, 254, 101, 150}, {255, 354, 151, 200},
    {355, 424, 201, 300}, {425, 604, 301, 500},
};

template <int N>
int interpolate(float c, const Breakpoint (&table)[N]) {
  for (const auto& bp : table) {
    if (c >= bp.cLow && c <= bp.cHigh) {
      float t = (bp.iHigh - bp.iLow) / (bp.cHigh - bp.cLow);
      return static_cast<int>(std::lround(t * (c - bp.cLow) + bp.iLow));
    }
  }
  return -1;
}

}  // namespace

int fromPm25(float c) { return interpolate(c, kPm25); }
int fromPm10(float c) { return interpolate(c, kPm10); }

int fromSample(float pm25, float pm10) {
  int a = fromPm25(pm25), b = fromPm10(pm10);
  return a > b ? a : b;
}

const char* label(int aqi) {
  if (aqi < 0) return "-";
  if (aqi <= 50) return "Good";
  if (aqi <= 100) return "Moderate";
  if (aqi <= 150) return "USG";
  if (aqi <= 200) return "Unhealthy";
  if (aqi <= 300) return "Very Unhealthy";
  return "Hazardous";
}

}  // namespace aqi
