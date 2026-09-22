// Firetracker — solar/battery SDS011 particulate sensor node (ESP32-C3).
// Wakes on a duty cycle, warms the fan, medians a burst of PM readings and
// posts them to the homelab endpoint that Firescreen renders from.

#include <WiFi.h>
#include <HTTPClient.h>
#include "SdsDustSensor.h"
#include "secrets.h"

// --- Tuning ---------------------------------------------------------------
const int   rxPin        = 5;      // SDS011 TXD -> ESP32 RX
const int   txPin        = 4;      // SDS011 RXD -> ESP32 TX
// Nova specs ~30s for the fan to fully exchange the chamber air; sampling then
// runs through the stable tail (~44s total fan-on per cycle).
const uint32_t WARMUP_MS = 30000;  // fan spin-up before we trust a reading
const int   DISCARD_COUNT = 2;     // toss the first reads after warm-up
const int   SAMPLE_COUNT = 12;     // readings to median-average per cycle
const uint32_t SAMPLE_GAP_MS = 1000;
const uint32_t MEASURE_INTERVAL_MS = 600000UL; // ~10 min between readings
const uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;

// --- Power-bank keepalive -------------------------------------------------
// This cheap bank uses an *average*-current cutoff: it integrates draw over a
// window and shuts off if the average stays low — so brief pulses can't fool it.
// The only firmware-only defense is a sustained load, so we keep Wi-Fi
// associated with modem-sleep disabled (~80-100mA continuous). That means NO
// deep sleep: the chip stays awake and paces readings with millis() instead.
// This trades battery life for keeping the bank online 100% without hardware.

HardwareSerial& sdsSerial = Serial1;
SdsDustSensor sds(sdsSerial);

uint32_t lastMeasure = 0;

void  ensureWiFi();
void  takeAndSendReading();
float median(float* arr, int n);

void setup() {
  setCpuFrequencyMhz(80);

  // Put the sensor into a known state once: query reporting + continuous working
  // period (0) so we own its schedule, then sleep it so the fan stays OFF until
  // a reading. (Wake first — a sleeping sensor silently drops config writes.)
  sdsSerial.begin(9600, SERIAL_8N1, rxPin, txPin);
  sds.begin();
  sds.wakeup();
  sds.setQueryReportingMode();
  sds.setContinuousWorkingPeriod();
  sds.sleep();

  // Bring Wi-Fi up and hold it: setSleep(false) keeps the radio in continuous RX
  // so the bank always sees a real load. Auto-reconnect keeps that draw alive if
  // the AP blips. This sustained current is the whole point — don't "optimize" it
  // away or the bank will cut off.
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false);
  ensureWiFi();

  // Take one reading right away, then pace the rest from loop().
  takeAndSendReading();
  lastMeasure = millis();
}

void loop() {
  ensureWiFi();  // keep the sustained keepalive load alive

  if (millis() - lastMeasure >= MEASURE_INTERVAL_MS) {
    takeAndSendReading();
    lastMeasure = millis();
  }
  delay(500);
}

// Block until associated (or timeout). Reconnect is best-effort; even while
// reconnecting the radio stays powered, so the keepalive load persists.
void ensureWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  WiFi.begin(ssid, password);
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
    delay(200);
  }
}

void takeAndSendReading() {
  sds.wakeup();                     // fan on — the only time it runs
  delay(WARMUP_MS);                 // let airflow stabilize

  float pm25[SAMPLE_COUNT], pm10[SAMPLE_COUNT];
  int   good = 0;
  // Take DISCARD_COUNT + SAMPLE_COUNT reads; ignore the first few so a
  // not-quite-settled first reading never enters the median.
  for (int i = 0; i < DISCARD_COUNT + SAMPLE_COUNT && good < SAMPLE_COUNT; i++) {
    PmResult pm = sds.queryPm();
    if (pm.isOk() && i >= DISCARD_COUNT) {
      pm25[good] = pm.pm25;
      pm10[good] = pm.pm10;
      good++;
    }
    delay(SAMPLE_GAP_MS);
  }

  sds.sleep();                      // fan/laser off — this is what saves the sensor

  if (good == 0) return;            // no valid readings; try again next interval

  // Median rejects the occasional outlier spike better than a plain mean.
  float pm25Med = median(pm25, good);
  float pm10Med = median(pm10, good);

  ensureWiFi();
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");
    String payload = "{\"pm25\": " + String(pm25Med, 1) +
                     ", \"pm10\": " + String(pm10Med, 1) + "}";
    http.POST(payload);
    http.end();
  }
}

// Insertion sort + middle value. n is small (<= SAMPLE_COUNT), so this is cheap.
float median(float* arr, int n) {
  for (int i = 1; i < n; i++) {
    float key = arr[i];
    int j = i - 1;
    while (j >= 0 && arr[j] > key) {
      arr[j + 1] = arr[j];
      j--;
    }
    arr[j + 1] = key;
  }
  if (n & 1) return arr[n / 2];
  return (arr[n / 2 - 1] + arr[n / 2]) / 2.0f;
}
