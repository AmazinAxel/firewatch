#include "homelab.h"

#include <HTTPClient.h>
#include <WiFiClient.h>

#include <memory>

#include "aqi.h"
#include "net.h"
#include "pm_parser.h"
#include "timeutil.h"

namespace homelab {
namespace {

constexpr size_t MAX_BODY = 256 * 1024;  // 7d is ~50 KB; hard safety cap

// The whole body is buffered (fits easily in RAM) because bucketing needs
// the time range before parsing: the end of the range comes from the Date
// header or, failing that, from the *last* sample in the body.
std::unique_ptr<char[]> readBody(HTTPClient& http, size_t& lenOut) {
  int size = http.getSize();
  if (size <= 0 || static_cast<size_t>(size) > MAX_BODY) {
    // Chunked or oversized: read up to the cap.
    size = MAX_BODY;
  }
  std::unique_ptr<char[]> buf(new (std::nothrow) char[size + 1]);
  if (!buf) return nullptr;

  WiFiClient* stream = http.getStreamPtr();
  size_t got = 0;
  uint32_t deadline = millis() + HTTP_TIMEOUT_MS;
  while (http.connected() && got < static_cast<size_t>(size) &&
         millis() < deadline) {
    int avail = stream->available();
    if (avail <= 0) {
      delay(5);
      continue;
    }
    int n = stream->readBytes(buf.get() + got,
                              min(static_cast<size_t>(avail),
                                  static_cast<size_t>(size) - got));
    if (n <= 0) break;
    got += n;
    deadline = millis() + HTTP_TIMEOUT_MS;
  }
  buf[got] = '\0';
  lenOut = got;
  return buf;
}

// Timestamp of the last sample: scan backwards for the final "timestamp" key.
int64_t lastTimestampMs(const char* body, size_t len) {
  for (const char* p = body + len; p > body; --p) {
    if (strncmp(p, "\"timestamp\"", 11) == 0) {
      const char* colon = strchr(p, ':');
      return colon ? strtoll(colon + 1, nullptr, 10) : 0;
    }
  }
  return 0;
}

}  // namespace

void fetch(bool range7d, History& hist, AirQuality& aq, time_t& nowOut) {
  IPAddress ip = net::homelabIp();
  if (ip == IPAddress()) {
    Serial.println("homelab: resolve failed");
    return;
  }

  char url[64];
  snprintf(url, sizeof(url), "http://%s%s", ip.toString().c_str(),
           range7d ? "/getdataFrom7Days" : "/getdata");

  WiFiClient client;
  HTTPClient http;
  http.useHTTP10(true);  // no chunked framing in the buffered body
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.setConnectTimeout(HTTP_TIMEOUT_MS);
  const char* dateHeader[] = {"Date"};
  http.collectHeaders(dateHeader, 1);
  if (!http.begin(client, url)) return;

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("homelab: HTTP %d\n", code);
    http.end();
    net::invalidateHomelabIp();  // stale cached IP? re-resolve next wake
    return;
  }

  time_t headerNow = 0;
  if (http.hasHeader("Date"))
    headerNow = timeutil::parseHttpDate(http.header("Date").c_str());

  size_t len = 0;
  auto body = readBody(http, len);
  http.end();
  if (!body || len == 0) {
    Serial.println("homelab: empty body");
    return;
  }

  int64_t endMs = headerNow ? static_cast<int64_t>(headerNow) * 1000
                            : lastTimestampMs(body.get(), len);
  if (endMs == 0) {
    Serial.println("homelab: no usable timestamp");
    return;
  }
  int64_t spanMs = (range7d ? 7 * 24 : 24) * 3600LL * 1000;

  hist.range7d = range7d;
  hist.buckets = range7d ? CHART_BUCKETS_7D : CHART_BUCKETS_24H;
  hist.startT = static_cast<time_t>((endMs - spanMs) / 1000);
  hist.endT = static_cast<time_t>(endMs / 1000);

  PmStreamParser parser;
  parser.beginBuckets(endMs - spanMs, endMs, hist.buckets, hist.v);
  parser.feed(body.get(), len);

  Serial.printf("homelab: %u samples\n", parser.sampleCount());
  if (!parser.hasLast()) return;

  hist.ok = true;
  const auto& s = parser.last();
  aq.pm25Aqi = aqi::fromPm25(static_cast<float>(s.pm25));
  aq.pm10Aqi = aqi::fromPm10(static_cast<float>(s.pm10));
  aq.aqi = max(aq.pm25Aqi, aq.pm10Aqi);
  aq.ok = aq.aqi >= 0;

  if (nowOut == 0 && headerNow) nowOut = headerNow;
  if (nowOut == 0) nowOut = static_cast<time_t>(s.tsMs / 1000);
}

}  // namespace homelab
