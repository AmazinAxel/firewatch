// EPA AQI math and category labels, ported 1:1 from the homelab webserver
// (flake/hosts/alechomelab/webserver/page.html) so device and web page agree.
// Pure C++ (no Arduino deps) so it is unit-testable on the host.
#pragma once

namespace aqi {

// AQI from a pm2.5 concentration (ug/m3). Returns -1 if out of table range.
int fromPm25(float concentration);

// AQI from a pm10 concentration (ug/m3). Returns -1 if out of table range.
int fromPm10(float concentration);

// Headline AQI for a sample: max of both pollutant AQIs (-1 if both invalid).
int fromSample(float pm25, float pm10);

// EPA category label: "Good", "Moderate", "USG", "Unhealthy",
// "Very Unhealthy", "Hazardous". Returns "-" for negative (invalid) AQI.
const char* label(int aqi);

}  // namespace aqi
