// Incremental parser for the homelab's /getdata JSON:
//   [{"pm25":1.2,"pm10":1.9,"timestamp":1783798508556}, ...]
//
// The 7-day response is ~50 KB; instead of building a JSON document we feed
// the HTTP stream through this parser chunk by chunk. Each sample's headline
// AQI is folded straight into fixed-size max-buckets for the chart, and the
// last raw sample is kept for the "current AQI" panel.
//
// Pure C++ (no Arduino deps) so it is unit-testable on the host.
#pragma once

#include <cstddef>
#include <cstdint>

class PmStreamParser {
 public:
  struct Sample {
    double pm25 = 0, pm10 = 0;
    int64_t tsMs = 0;
  };

  // Bucket samples into `count` max-buckets spanning [t0Ms, t1Ms).
  // `out` must hold `count` entries; they are initialized to -1 (empty).
  void beginBuckets(int64_t t0Ms, int64_t t1Ms, uint16_t count, int16_t* out);

  void feed(const char* data, size_t len);

  uint32_t sampleCount() const { return samples_; }
  bool hasLast() const { return samples_ > 0; }
  const Sample& last() const { return last_; }

 private:
  void finishObject();

  // One JSON object is <100 chars; anything longer is malformed and dropped.
  char obj_[128];
  size_t objLen_ = 0;
  bool inObject_ = false;

  int16_t* buckets_ = nullptr;
  uint16_t bucketCount_ = 0;
  int64_t t0Ms_ = 0, t1Ms_ = 0;

  Sample last_;
  uint32_t samples_ = 0;
};
