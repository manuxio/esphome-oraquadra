#pragma once

// Diagnostic test pattern — a fully static frame that should NEVER change
// pixel by pixel between frames. Use this to isolate hardware/data-line
// flicker from software flicker:
//   - paint a hue gradient on the inner 14×14 (each pixel = unique colour)
//   - ring the cornice with bright primaries (R/G/B repeating)
//   - corner markers always pure white
// If you see ANY pixel flash a different colour while in this mode, the
// problem is on the data wire (level mismatch, length, ground, EMI). The
// software writes the same byte sequence every loop, so any transient is
// signal corruption between RMT and the LEDs.

#include <cstdint>
#include "effect.h"

namespace esphome {
namespace oraquadra {

class DiagEffect final : public Effect {
 public:
  const char *name() const override { return "Diagnostic"; }

  void update(uint32_t /*now_ms*/, Matrix &m, const ClockState & /*s*/) override {
    // Inner 14x14 hue gradient (high entropy → any bit flip stands out).
    for (uint8_t y = 1; y < 15; y++) {
      for (uint8_t x = 1; x < 15; x++) {
        const uint8_t hue = static_cast<uint8_t>((x * 18 + y * 11) & 0xFF);
        m.set_pixel(x, y, hsv_(hue, 255, 200));
      }
    }
    // Cornice: cycle R/G/B repeating around the perimeter.
    static const Color RING[3] = {
        Color{255, 0, 0}, Color{0, 255, 0}, Color{0, 0, 255},
    };
    for (uint8_t i = 0; i < Matrix::CORNICE_LEN; i++) {
      m.set_cornice(i, RING[i % 3]);
    }
    // Corner markers — always pure white. Easy reference points.
    m.set_pixel(0, 0, Color{255, 255, 255});
    m.set_pixel(15, 0, Color{255, 255, 255});
    m.set_pixel(0, 15, Color{255, 255, 255});
    m.set_pixel(15, 15, Color{255, 255, 255});
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
