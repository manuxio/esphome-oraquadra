#pragma once

// Drop: concentric circular waves expanding from the matrix center. Each
// wave fades out as it expands. Time text painted on top.

#include "effect.h"
#include <cmath>

namespace esphome {
namespace oraquadra {

class DropEffect final : public Effect {
 public:
  const char *name() const override { return "Drop"; }

  void update(uint32_t now_ms, Matrix &m, const ClockState &s) override {
    if (now_ms - last_step_ < 60) {
      paint_(m, s);
      return;
    }
    last_step_ = now_ms;
    radius_ += 0.4f;
    if (radius_ > MAX_RADIUS) radius_ = 0.0f;
    paint_(m, s);
  }

 private:
  static constexpr float MAX_RADIUS = 12.0f;
  static constexpr float CX = 7.5f;
  static constexpr float CY = 7.5f;

  void paint_(Matrix &m, const ClockState &s) {
    // Wave intensity peaks at current radius, falls off ~1 pixel each side.
    for (uint8_t y = 0; y < Matrix::HEIGHT; y++) {
      for (uint8_t x = 0; x < Matrix::WIDTH; x++) {
        float dx = x - CX;
        float dy = y - CY;
        float dist = std::sqrt(dx * dx + dy * dy);
        float diff = std::fabs(dist - radius_);
        if (diff > 1.5f) continue;
        // Intensity falls off with diff. Also fades as radius grows (the wave
        // "weakens" expanding outward).
        float wave = 1.0f - (diff / 1.5f);
        float fade = 1.0f - (radius_ / MAX_RADIUS);
        uint8_t scale = static_cast<uint8_t>(255 * wave * fade);
        Color c{static_cast<uint8_t>((s.color.r * scale) >> 8),
                static_cast<uint8_t>((s.color.g * scale) >> 8),
                static_cast<uint8_t>((s.color.b * scale) >> 8)};
        m.set_pixel(x, y, c);
      }
    }
    if (s.language != nullptr) {
      s.language->render_time(m, s.hour, s.minute, s.color);
    }
  }

  float radius_{0.0f};
  uint32_t last_step_{0};
};

}  // namespace oraquadra
}  // namespace esphome
