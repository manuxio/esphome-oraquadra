#pragma once

// Matrix rain: green vertical drops fall from top to bottom on dark background,
// each drop has a fading trail. Time text painted on top in user color.

#include "effect.h"
#include <cstdlib>

namespace esphome {
namespace oraquadra {

class MatrixEffect final : public Effect {
 public:
  const char *name() const override { return "Matrix"; }

  void update(uint32_t now_ms, Matrix &m, const ClockState &s) override {
    if (now_ms - last_step_ < 80) {
      // Re-paint same frame: trail decays slowly.
      paint_(m, s);
      return;
    }
    last_step_ = now_ms;

    // Dim every drop slightly each step (trail effect).
    for (auto &d : drops_) {
      if (d.y < 0) {
        // Spawn at top with random column.
        d.x = static_cast<int8_t>(rand() % Matrix::WIDTH);
        d.y = 0;
        d.speed = 1 + (rand() % 2);
      } else {
        d.y += d.speed;
        if (d.y >= Matrix::HEIGHT + TRAIL_LEN) d.y = -1;  // recycle
      }
    }

    paint_(m, s);
  }

 private:
  static constexpr uint8_t NUM_DROPS = 12;
  static constexpr uint8_t TRAIL_LEN = 6;

  struct Drop {
    int8_t x{-1};
    int8_t y{-1};
    uint8_t speed{1};
  };

  void paint_(Matrix &m, const ClockState &s) {
    // Drops with trails.
    for (auto &d : drops_) {
      for (uint8_t i = 0; i < TRAIL_LEN; i++) {
        int8_t y = d.y - i;
        if (y < 0 || y >= Matrix::HEIGHT) continue;
        uint8_t intensity = static_cast<uint8_t>(255 - (i * 255 / TRAIL_LEN));
        Color green{0, intensity, 0};
        m.set_pixel(static_cast<uint8_t>(d.x), static_cast<uint8_t>(y), green);
      }
    }
    // Time text on top in user color.
    if (s.language != nullptr) {
      s.language->render_time(m, s.hour, s.minute, s.color);
    }
  }

  Drop drops_[NUM_DROPS];
  uint32_t last_step_{0};
};

}  // namespace oraquadra
}  // namespace esphome
