#pragma once

// Digital: HH on the top half (rows 0-6), MM on the bottom (rows 9-15), with
// a single LED at the matrix centre that blinks once per second when
// blink-seconds is on.

#include <cstdint>
#include "effect.h"
#include "fonts.h"

namespace esphome {
namespace oraquadra {

class DigitalEffect final : public Effect {
 public:
  const char *name() const override { return "Digital"; }

  void update(uint32_t /*now_ms*/, Matrix &m, const ClockState &s) override {
    uint8_t hourTens = s.hour / 10;
    uint8_t hourOnes = s.hour % 10;
    uint8_t minTens = s.minute / 10;
    uint8_t minOnes = s.minute % 10;

    // The original picks x=3, y=0 for hour digits and x=3, y=9 for minutes.
    // The two digits are placed at x and x+6 (5-col glyph + 1-col gap).
    paint_digit_(m, 3, 0, hourTens, s.color);
    paint_digit_(m, 9, 0, hourOnes, s.color);
    paint_digit_(m, 3, 9, minTens, s.color);
    paint_digit_(m, 9, 9, minOnes, s.color);

    // Center dot at (8, 8) blinks on even seconds when blink-seconds is on.
    if (s.blink_seconds && (s.second % 2) == 0) {
      m.set_pixel(8, 8, s.color);
    }
  }

 private:
  static void paint_digit_(Matrix &m, uint8_t x, uint8_t y, uint8_t digit, Color color) {
    char c = static_cast<char>('0' + digit);
    const uint8_t *glyph = font5x7_glyph(c);
    for (uint8_t col = 0; col < FONT5x7_WIDTH; col++) {
      int screenX = static_cast<int>(x) + col;
      if (screenX < 0 || screenX >= static_cast<int>(Matrix::WIDTH)) continue;
      uint8_t bits = glyph[col];
      for (uint8_t row = 0; row < FONT5x7_HEIGHT; row++) {
        if (bits & (1 << row)) {
          int screenY = static_cast<int>(y) + row;
          if (screenY < 0 || screenY >= static_cast<int>(Matrix::HEIGHT)) continue;
          m.set_pixel(static_cast<uint8_t>(screenX), static_cast<uint8_t>(screenY), color);
        }
      }
    }
  }
};

}  // namespace oraquadra
}  // namespace esphome
