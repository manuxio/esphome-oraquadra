#pragma once

// Rainbow: show the Italian time, but each frame uses a different hue rotated
// from the previous. The whole text shifts color smoothly over time.

#include "effect.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace oraquadra {

class RainbowEffect final : public Effect {
 public:
  const char *name() const override { return "Rainbow"; }

  void update(uint32_t now_ms, Matrix &m, const ClockState &s) override {
    if (s.language == nullptr) return;

    // ~6 second full hue cycle (256 hue × 24 ms ≈ 6.1 s).
    if (now_ms - last_step_ >= 24) {
      last_step_ = now_ms;
      hue_ += 1;
    }

    Color c = hsv_(hue_);
    s.language->render_time(m, s.hour, s.minute, c);
  }

 private:
  static Color hsv_(uint8_t h) {
    // Compact HSV at full S/V — enough for the rainbow look.
    uint8_t region = h / 43;
    uint8_t rem = (h - region * 43) * 6;
    switch (region) {
      case 0: return Color{255, rem, 0};
      case 1: return Color{static_cast<uint8_t>(255 - rem), 255, 0};
      case 2: return Color{0, 255, rem};
      case 3: return Color{0, static_cast<uint8_t>(255 - rem), 255};
      case 4: return Color{rem, 0, 255};
      default: return Color{255, 0, static_cast<uint8_t>(255 - rem)};
    }
  }

  uint32_t last_step_{0};
  uint8_t hue_{0};
};

}  // namespace oraquadra
}  // namespace esphome
