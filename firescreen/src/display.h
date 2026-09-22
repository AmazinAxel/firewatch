// E-ink pipeline built on the raw GxEPD2_750_T7 driver (UC8179) plus an
// in-RAM GFXcanvas1 we render into.
//
// Partial ("fast") refreshes need the controller's previous-frame buffer to
// hold what is actually on the glass, but the panel is hibernated between
// wakes and loses its RAM. We therefore persist the rendered frame
// (RLE-compressed, ~4 KB) to LittleFS on every cycle and restore it into
// both controller buffers right after init — this is what makes flicker-free
// partial refreshes work across deep sleep.
#pragma once

#include <Adafruit_GFX.h>

namespace display {

// Init SPI, panel and LittleFS; restore the previous frame if one is saved.
// Returns true when the restore succeeded (partial refreshes are valid).
bool begin();

GFXcanvas1& canvas();

// True once the canvas holds a real frame (restored or pushed) — i.e. a
// partial refresh would diff against known content.
bool hasContent();

// Draw the refresh icon in its white box at the cross intersection and
// update just that window. No-op unless hasContent().
void showSpinner();

// Push the whole canvas to the glass. full=false does a fast partial update.
void push(bool full);

// Persist the current canvas for the next wake and hibernate the panel.
void finish();

}  // namespace display
