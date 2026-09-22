// Dashboard renderer: quadrant layout, overflow-proof text drawing, icons.
#pragma once

#include <Adafruit_GFX.h>

#include <initializer_list>

#include "model.h"

namespace render {

// The generated Iosevka fonts keep the degree sign in slot 0x7f (DEL).
constexpr char DEG_CH = '\x7f';

constexpr uint16_t INK = 0;    // GFXcanvas1 bit clear = black on the panel
constexpr uint16_t PAPER = 1;  // bit set = white

struct TextBox {
  int16_t x, y, w, h;
};
enum class HAlign { Left, Center, Right };

// Draw `s` inside `box` using the largest font of `ladder` that fits both
// dimensions; if even the smallest overflows, characters are dropped from
// the end. This is the single path for all dynamic strings, guaranteeing
// nothing ever escapes its bounding box.
void textFitted(GFXcanvas1& c, const TextBox& box, const char* s,
                std::initializer_list<const GFXfont*> ladder,
                HAlign align = HAlign::Left);

// Render the full dashboard (weather, chart, AQI panels, cross, stamp).
void draw(GFXcanvas1& c, const Dashboard& d);

// White-padded box with the refresh icon, centered on the cross
// intersection (drawn while data is being fetched).
void drawSpinnerBox(GFXcanvas1& c);

}  // namespace render
