#include "timeutil.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "config.h"

namespace timeutil {

void setupTz() {
  setenv("TZ", TZ_POSIX, 1);
  tzset();
}

long daysFromCivil(int y, int m, int d) {
  y -= m <= 2;
  long era = (y >= 0 ? y : y - 399) / 400;
  unsigned yoe = static_cast<unsigned>(y - era * 400);
  unsigned doy = (153u * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097L + static_cast<long>(doe) - 719468L;
}

int weekdayFromYmd(int y, int m, int d) {
  long days = daysFromCivil(y, m, d);
  return static_cast<int>(((days % 7) + 11) % 7);  // 1970-01-01 was a Thursday
}

const char* weekdayName(int wd) {
  static const char* const kNames[] = {"Sun", "Mon", "Tue", "Wed",
                                       "Thu", "Fri", "Sat"};
  return (wd >= 0 && wd < 7) ? kNames[wd] : "?";
}

time_t parseHttpDate(const char* s) {
  // "Sun, 12 Jul 2026 19:03:12 GMT"
  static const char kMonths[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
  char mon[4] = {};
  int d, y, hh, mm, ss;
  if (std::sscanf(s, "%*3s, %d %3s %d %d:%d:%d", &d, mon, &y, &hh, &mm, &ss) != 6)
    return 0;
  const char* p = std::strstr(kMonths, mon);
  if (!p) return 0;
  int m = static_cast<int>(p - kMonths) / 3 + 1;
  return static_cast<time_t>(daysFromCivil(y, m, d)) * 86400 + hh * 3600 +
         mm * 60 + ss;
}

void fmtHour12(int hour, char* out) {
  int h = hour % 12;
  if (h == 0) h = 12;
  std::sprintf(out, "%d%s", h, hour < 12 ? "am" : "pm");
}

}  // namespace timeutil
