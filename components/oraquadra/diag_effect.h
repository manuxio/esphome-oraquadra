#pragma once

// Diagnostic test pattern — 16 vertical rainbow bars that scroll
// horizontally at 20 fps. Each bar is one solid hue; bars are 16 hue
// units apart so the whole frame is high-contrast and any out-of-place
// pixel is instantly obvious against its monochrome column. The pattern
// animates predictably, so you can recognise both static glitches (a
// pixel that doesn't match its column's hue) and timing glitches (a
// bar tearing or a row offset).

#include <cstdint>
#include "effect.h"

namespace esphome {
namespace oraquadra {

class DiagEffect final : public Effect {
 public:
  const char *name() const override { return "Diagnostic"; }

  void update(uint32_t now_ms, Matrix &m, const ClockState & /*s*/) override {
    // Bars shift exactly one column every second — discrete steps, not
    // a smooth scroll. Easier on the eyes for spotting transient glitches.
    const uint8_t phase = static_cast<uint8_t>(now_ms / 1000);
    for (uint8_t x = 0; x < Matrix::WIDTH; x++) {
      // Bars are 16 hue units apart so the 16 columns span the full hue
      // wheel (16*16 = 256 → wraps at the edge).
      const uint8_t hue = static_cast<uint8_t>((x + phase) * 16);
      const Color c = hsv_(hue, 255, 255);
      for (uint8_t y = 0; y < Matrix::HEIGHT; y++) {
        m.set_pixel(x, y, c);
      }
    }
  }

 private:
  static Color hsv_(uint8_t h, uint8_t s, uint8_t v) {
    if (s == 0) return Color{v, v, v};
    uint8_t region = h / 43;
    uint8_t remainder = (h - region * 43) * 6;
    uint8_t p = static_cast<uint8_t>((v * (255 - s)) >> 8);
    uint8_t q = static_cast<uint8_t>((v * (255 - ((s * remainder) >> 8))) >> 8);
    uint8_t t = static_cast<uint8_t>((v * (255 - ((s * (255 - remainder)) >> 8))) >> 8);
    switch (region) {
      case 0: return Color{v, t, p};
      case 1: return Color{q, v, p};
      case 2: return Color{p, v, t};
      case 3: return Color{p, q, v};
      case 4: return Color{t, p, v};
      default: return Color{v, p, q};
    }
  }
};

}  // namespace oraquadra
}  // namespace esphome
