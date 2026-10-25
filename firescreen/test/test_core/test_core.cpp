// Host-side tests for the pure-logic modules (no Arduino/hardware deps).
// Run with: pio test -e native
//
// The .cpp files are included directly because test_build_src is off for
// the native environment (the rest of src/ needs the Arduino framework).
#include "../../src/aqi.cpp"
#include "../../src/pm_parser.cpp"
#include "../../src/timeutil.cpp"

#include <unity.h>

#include <cstring>
#include <string>

void test_aqi_pm25_breakpoints() {
  TEST_ASSERT_EQUAL(0, aqi::fromPm25(0.0f));
  TEST_ASSERT_EQUAL(4, aqi::fromPm25(1.0f));
  TEST_ASSERT_EQUAL(50, aqi::fromPm25(12.0f));
  TEST_ASSERT_EQUAL(100, aqi::fromPm25(35.4f));
  TEST_ASSERT_EQUAL(150, aqi::fromPm25(55.4f));
  TEST_ASSERT_EQUAL(500, aqi::fromPm25(500.4f));
  TEST_ASSERT_EQUAL(-1, aqi::fromPm25(9999.0f));
}

void test_aqi_pm10_breakpoints() {
  TEST_ASSERT_EQUAL(1, aqi::fromPm10(1.0f));
  TEST_ASSERT_EQUAL(50, aqi::fromPm10(54.0f));
  TEST_ASSERT_EQUAL(100, aqi::fromPm10(154.0f));
  TEST_ASSERT_EQUAL(-1, aqi::fromPm10(9999.0f));
}

void test_aqi_between_table_rows() {
  TEST_ASSERT_EQUAL(50, aqi::fromPm25(12.05f));
  TEST_ASSERT_EQUAL(50, aqi::fromPm10(54.5f));
}

void test_aqi_sample_takes_max() {
  // pm10=200 -> AQI ~123 beats pm25=10 -> AQI ~42
  TEST_ASSERT_EQUAL(aqi::fromPm10(200.0f), aqi::fromSample(10.0f, 200.0f));
}

void test_aqi_labels() {
  TEST_ASSERT_EQUAL_STRING("Good", aqi::label(50));
  TEST_ASSERT_EQUAL_STRING("Moderate", aqi::label(100));
  TEST_ASSERT_EQUAL_STRING("USG", aqi::label(150));
  TEST_ASSERT_EQUAL_STRING("Unhealthy", aqi::label(200));
  TEST_ASSERT_EQUAL_STRING("Very Unhealthy", aqi::label(300));
  TEST_ASSERT_EQUAL_STRING("Hazardous", aqi::label(301));
  TEST_ASSERT_EQUAL_STRING("-", aqi::label(-1));
}

void test_parser_basic_and_chunked() {
  const std::string body =
      R"([{"pm25":1.2,"pm10":1.9,"timestamp":1000},)"
      R"({"pm25":40.0,"pm10":2.0,"timestamp":2000},)"
      R"({"pm25":2.0,"pm10":3.0,"timestamp":3500}])";

  // Feed in awkward 7-byte chunks to prove incremental parsing works.
  PmStreamParser p;
  int16_t buckets[4];
  p.beginBuckets(0, 4000, 4, buckets);
  for (size_t i = 0; i < body.size(); i += 7)
    p.feed(body.data() + i, std::min<size_t>(7, body.size() - i));

  TEST_ASSERT_EQUAL(3, p.sampleCount());
  TEST_ASSERT_EQUAL_FLOAT(2.0f, static_cast<float>(p.last().pm25));
  TEST_ASSERT_EQUAL(3500, (int)p.last().tsMs);

  TEST_ASSERT_EQUAL(aqi::fromSample(1.2f, 1.9f), buckets[1]);   // ts=1000
  TEST_ASSERT_EQUAL(aqi::fromSample(40.0f, 2.0f), buckets[2]);  // ts=2000
  TEST_ASSERT_EQUAL(aqi::fromSample(2.0f, 3.0f), buckets[3]);   // ts=3500
  TEST_ASSERT_EQUAL(-1, buckets[0]);                            // empty
}

void test_parser_bucket_keeps_max() {
  PmStreamParser p;
  int16_t buckets[1];
  p.beginBuckets(0, 1000, 1, buckets);
  const char* body =
      R"([{"pm25":5,"pm10":5,"timestamp":100},)"
      R"({"pm25":50,"pm10":5,"timestamp":200},)"
      R"({"pm25":6,"pm10":6,"timestamp":300}])";
  p.feed(body, strlen(body));
  TEST_ASSERT_EQUAL(aqi::fromSample(50, 5), buckets[0]);
}

void test_parser_ignores_out_of_range() {
  PmStreamParser p;
  int16_t buckets[2];
  p.beginBuckets(1000, 2000, 2, buckets);
  const char* body = R"([{"pm25":5,"pm10":5,"timestamp":50}])";
  p.feed(body, strlen(body));
  TEST_ASSERT_EQUAL(-1, buckets[0]);
  TEST_ASSERT_EQUAL(-1, buckets[1]);
  TEST_ASSERT_EQUAL(1, p.sampleCount());  // still counted as latest sample
}

void test_http_date_parse() {
  // 2026-07-12 19:03:12 UTC
  time_t t = timeutil::parseHttpDate("Sun, 12 Jul 2026 19:03:12 GMT");
  TEST_ASSERT_EQUAL(1783882992, (long)t);
  TEST_ASSERT_EQUAL(0, (long)timeutil::parseHttpDate("garbage"));
}

void test_weekday() {
  TEST_ASSERT_EQUAL(0, timeutil::weekdayFromYmd(2026, 7, 12));  // Sunday
  TEST_ASSERT_EQUAL_STRING("Sun", timeutil::weekdayName(0));
  TEST_ASSERT_EQUAL_STRING("Sat", timeutil::weekdayName(6));
}

void test_hour_format() {
  char buf[8];
  timeutil::fmtHour12(0, buf);
  TEST_ASSERT_EQUAL_STRING("12am", buf);
  timeutil::fmtHour12(1, buf);
  TEST_ASSERT_EQUAL_STRING("1am", buf);
  timeutil::fmtHour12(12, buf);
  TEST_ASSERT_EQUAL_STRING("12pm", buf);
  timeutil::fmtHour12(23, buf);
  TEST_ASSERT_EQUAL_STRING("11pm", buf);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_aqi_pm25_breakpoints);
  RUN_TEST(test_aqi_pm10_breakpoints);
  RUN_TEST(test_aqi_between_table_rows);
  RUN_TEST(test_aqi_sample_takes_max);
  RUN_TEST(test_aqi_labels);
  RUN_TEST(test_parser_basic_and_chunked);
  RUN_TEST(test_parser_bucket_keeps_max);
  RUN_TEST(test_parser_ignores_out_of_range);
  RUN_TEST(test_http_date_parse);
  RUN_TEST(test_weekday);
  RUN_TEST(test_hour_format);
  return UNITY_END();
}
