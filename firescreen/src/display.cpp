#include "display.h"

#include <GxEPD2_BW.h>
#include <LittleFS.h>
#include <SPI.h>

#include "config.h"
#include "render.h"

namespace display {
namespace {

constexpr size_t FRAME_BYTES = SCREEN_W / 8 * SCREEN_H;
constexpr const char* FRAME_FILE = "/frame.rle";

GxEPD2_750_T7 epd(PIN_EPD_CS, PIN_EPD_DC, PIN_EPD_RST, PIN_EPD_BUSY);
GFXcanvas1 gfx(SCREEN_W, SCREEN_H);
bool sHasContent = false;

// --- frame persistence -----------------------------------------------------
// Byte-level RLE: [count 1..255][value]. A dashboard frame is mostly long
// white runs, compressing 48 KB to a few KB — small enough that daily flash
// wear is negligible at a 5-minute cadence.

bool saveFrame(const uint8_t* buf) {
  File f = LittleFS.open("/frame.tmp", "w");
  if (!f) return false;
  uint8_t out[512];
  size_t o = 0;
  size_t i = 0;
  while (i < FRAME_BYTES) {
    uint8_t v = buf[i];
    size_t run = 1;
    while (run < 255 && i + run < FRAME_BYTES && buf[i + run] == v) ++run;
    out[o++] = static_cast<uint8_t>(run);
    out[o++] = v;
    if (o >= sizeof(out) - 2) {
      f.write(out, o);
      o = 0;
    }
    i += run;
  }
  if (o) f.write(out, o);
  f.close();
  LittleFS.remove(FRAME_FILE);
  return LittleFS.rename("/frame.tmp", FRAME_FILE);
}

bool loadFrame(uint8_t* buf) {
  File f = LittleFS.open(FRAME_FILE, "r");
  if (!f) return false;
  size_t pos = 0;
  uint8_t chunk[512];
  int n;
  while ((n = f.read(chunk, sizeof(chunk))) > 0) {
    for (int i = 0; i + 1 < n; i += 2) {
      size_t run = chunk[i];
      if (pos + run > FRAME_BYTES) {
        f.close();
        return false;
      }
      memset(buf + pos, chunk[i + 1], run);
      pos += run;
    }
    if (n % 2) {  // run length split across chunks — rewind one byte
      f.seek(f.position() - 1);
    }
  }
  f.close();
  return pos == FRAME_BYTES;
}

// Write `buf` into both controller planes so the next partial refresh diffs
// against the frame that is actually on the glass. The UC8179 keeps OLD data
// in register 0x10 and NEW in 0x13; writeImageToPrevious is the only call
// that touches 0x10 (the generic writeImageAgain would write 0x13 twice and
// leave 0x10 as post-reset garbage -> full-screen noise on partial refresh).
void restoreControllerBuffers(const uint8_t* buf) {
  epd.writeImageToPrevious(buf, 0, 0, SCREEN_W, SCREEN_H);
  epd.writeImage(buf, 0, 0, SCREEN_W, SCREEN_H);
}

}  // namespace

bool begin() {
  SPI.begin(PIN_EPD_SCK, -1, PIN_EPD_MOSI, PIN_EPD_CS);
  LittleFS.begin(true);

  bool restored = loadFrame(gfx.getBuffer());
  epd.init(0, !restored /*initial*/, 10 /*reset ms, driver default*/, false);
  if (restored) {
    restoreControllerBuffers(gfx.getBuffer());
    sHasContent = true;
  } else {
    gfx.fillScreen(1);  // white
  }
  return restored;
}

GFXcanvas1& canvas() { return gfx; }

bool hasContent() { return sHasContent; }

// The controller copies NEW->OLD during a refresh, so after refreshing we
// rewrite the image to NEW (the driver's own display() does the same) to
// keep both planes equal to what is on the glass.

void showSpinner() {
  if (!sHasContent) return;
  int16_t bx = CROSS_X - SPINNER_BOX / 2;
  int16_t by = CROSS_Y - SPINNER_BOX / 2;
  render::drawSpinnerBox(gfx);
  epd.writeImagePart(gfx.getBuffer(), bx, by, SCREEN_W, SCREEN_H, bx, by,
                     SPINNER_BOX, SPINNER_BOX);
  epd.refresh(bx, by, SPINNER_BOX, SPINNER_BOX);
  epd.writeImagePart(gfx.getBuffer(), bx, by, SCREEN_W, SCREEN_H, bx, by,
                     SPINNER_BOX, SPINNER_BOX);
}

void push(bool full) {
  epd.writeImage(gfx.getBuffer(), 0, 0, SCREEN_W, SCREEN_H);
  epd.refresh(!full /*partial_update_mode*/);
  epd.writeImage(gfx.getBuffer(), 0, 0, SCREEN_W, SCREEN_H);
  sHasContent = true;
}

void finish() {
  saveFrame(gfx.getBuffer());
  epd.hibernate();
}

}  // namespace display
