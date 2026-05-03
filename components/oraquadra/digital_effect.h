#pragma once

// Digital: render the time as 4 numeric digits using the 5×7 font.
// Layout on the 16×16 matrix: HH on the top half (rows 1-7),
// MM on the bottom half (rows 9-15). Each pair of digits is centered.

#include "effect.h"
#include "fonts.h"

namespace esphome {
namespace oraquadra {

class DigitalEffect final : public Effect {
 public:
  const char *name() const override { return "Digitale"; }

  void update(uint32_t /*now_ms*/, Matrix &m, const ClockState &s) override {
    // Top: hour, two digits
    char hh_tens = '0' + (s.hour / 10);
    char hh_ones = '0' + (s.hour % 10);
    char mm_tens = '0' + (s.minute / 10);
    char mm_ones = '0' + (s.minute % 10);

    // Each glyph is 5 cols × 7 rows. Two glyphs + 1 col gap = 11 cols.
    // 16 - 11 = 5 cols margin, so left margin = 2 (centered-ish).
    paint_glyph_(m, 2, 1, hh_tens, s.color);
    paint_glyph_(m, 8, 1, hh_ones, s.color);
    paint_glyph_(m, 2, 9, mm_tens, s.color);
    paint_glyph_(m, 8, 9, mm_ones, s.color);
  }

 private:
  static void paint_glyph_(Matrix &m, uint8_t x0, uint8_t y0, char c, Color color) {
    const uint8_t *glyph = font5x7_glyph(c);
    for (uint8_t col = 0; col < FONT5x7_WIDTH; col++) {
      uint8_t bits = glyph[col];
      for (uint8_t row = 0; row < FONT5x7_HEIGHT; row++) {
        if (bits & (1 << row)) {
          m.set_pixel(x0 + col, y0 + row, color);
        }
      }
    }
  }
};

}  // namespace oraquadra
}  // namespace esphome
