#include "pm_parser.h"

#include <cstdlib>
#include <cstring>

#include "aqi.h"

void PmStreamParser::beginBuckets(int64_t t0Ms, int64_t t1Ms, uint16_t count,
                                  int16_t* out) {
  t0Ms_ = t0Ms;
  t1Ms_ = t1Ms;
  bucketCount_ = count;
  buckets_ = out;
  for (uint16_t i = 0; i < count; ++i) buckets_[i] = -1;
}

void PmStreamParser::feed(const char* data, size_t len) {
  for (size_t i = 0; i < len; ++i) {
    char c = data[i];
    if (!inObject_) {
      if (c == '{') {
        inObject_ = true;
        objLen_ = 0;
      }
      continue;
    }
    if (c == '}') {
      inObject_ = false;
      obj_[objLen_] = '\0';
      finishObject();
    } else if (objLen_ < sizeof(obj_) - 1) {
      obj_[objLen_++] = c;
    }
    // Overlong objects keep discarding chars until '}' — sample is dropped
    // by finishObject() when a key is missing.
  }
}

void PmStreamParser::finishObject() {
  // Keys always appear as "pm25":, "pm10":, "timestamp": — locate each and
  // parse the number that follows.
  auto value = [&](const char* key) -> const char* {
    const char* p = std::strstr(obj_, key);
    if (!p) return nullptr;
    p = std::strchr(p + std::strlen(key), ':');
    return p ? p + 1 : nullptr;
  };

  const char *v25 = value("\"pm25\""), *v10 = value("\"pm10\""),
             *vts = value("\"timestamp\"");
  if (!v25 || !v10 || !vts) return;

  Sample s;
  s.pm25 = std::strtod(v25, nullptr);
  s.pm10 = std::strtod(v10, nullptr);
  s.tsMs = std::strtoll(vts, nullptr, 10);
  last_ = s;
  ++samples_;

  if (!buckets_ || s.tsMs < t0Ms_ || s.tsMs >= t1Ms_) return;
  int idx = static_cast<int>((s.tsMs - t0Ms_) * bucketCount_ / (t1Ms_ - t0Ms_));
  if (idx < 0 || idx >= bucketCount_) return;

  int a = aqi::fromSample(static_cast<float>(s.pm25), static_cast<float>(s.pm10));
  if (a > buckets_[idx]) buckets_[idx] = static_cast<int16_t>(a);
}
