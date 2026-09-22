// Small time helpers: HTTP Date header parsing (our clock source — no SNTP
// round-trip needed), civil-date math, and the "1pm"/"12am" hour formatting
// used by the chart. Pure C++ except setupTz(); host-testable.
#pragma once

#include <ctime>

namespace timeutil {

// Install TZ_POSIX (America/Los_Angeles) so localtime() is correct.
void setupTz();

// Parse an RFC 7231 date ("Sun, 12 Jul 2026 19:03:12 GMT") to a UTC epoch.
// Returns 0 on parse failure.
time_t parseHttpDate(const char* s);

// Days since 1970-01-01 for a civil date (Howard Hinnant's algorithm).
long daysFromCivil(int y, int m, int d);

// 0=Sun..6=Sat for a civil date.
int weekdayFromYmd(int y, int m, int d);

// Three-letter weekday name for 0=Sun..6=Sat.
const char* weekdayName(int wd);

// Format an hour-of-day (0..23) as "12am", "1am", ..., "12pm", "11pm".
// `out` needs >= 5 bytes.
void fmtHour12(int hour, char* out);

}  // namespace timeutil
