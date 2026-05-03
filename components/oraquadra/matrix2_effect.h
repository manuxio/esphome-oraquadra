#pragma once

// Matrix2: drops descend through the matrix and "stick" on the time-word
// pixels — over ~10 s the words fill out drop-by-drop in user color, then
// reset. Variant of Matrix with the words progressively painted instead of
// always-on.

#include "effect.h"
#include <cstdlib>

namespace esphome {
namespace oraquadra {

class Matrix2Effect final : public Effect {
 public:
  const char *name() const override { return "Matrix2"; }

  void on_minute_change(uint8_t /*h*/, uint8_t /*m*/) override {
    // Re-roll drops at minute change.
    for (auto &d : drops_) d.y = -1;
    completion_pct_ = 0;
  }

  void update(uint32_t now_ms, Matrix &m, const ClockState &s) override {
    if (now_ms - last_step_ < 60) {
      paint_(m, s);
      return;
    }
    last_step_ = now_ms;

    for (auto &d : drops_) {
      if (d.y < 0) {
        d.x = static_cast<int8_t>(rand() % Matrix::WIDTH);
        d.y = 0;
      } else {
        d.y++;
        if (d.y >= Matrix::HEIGHT) d.y = -1;
      }
    }
    completion_pct_ = (completion_pct_ < 100) ? completion_pct_ + 1 : 100;
    paint_(m, s);
  }

 private:
  static constexpr uint8_t NUM_DROPS = 14;
  static constexpr uint8_t TRAIL_LEN = 4;

  struct Drop { int8_t x{-1}; int8_t y{-1}; };

  void paint_(Matrix &m, const ClockState &s) {
    // Drops with white-fade trails.
    for (auto &d : drops_) {
      for (uint8_t i = 0; i < TRAIL_LEN; i++) {
        int8_t y = d.y - i;
        if (y < 0 || y >= Matrix::HEIGHT) continue;
        uint8_t intensity = static_cast<uint8_t>(160 - (i * 160 / TRAIL_LEN));
        m.set_pixel(static_cast<uint8_t>(d.x), static_cast<uint8_t>(y),
                    Color{intensity, intensity, intensity});
      }
    }
    // Words painted at intensity proportional to completion (faded-in over time).
    if (s.language != nullptr) {
      uint8_t scale = static_cast<uint8_t>((completion_pct_ * 255) / 100);
      Color c{static_cast<uint8_t>((s.color.r * scale) >> 8),
              static_cast<uint8_t>((s.color.g * scale) >> 8),
              static_cast<uint8_t>((s.color.b * scale) >> 8)};
      s.language->render_time(m, s.hour, s.minute, c);
    }
  }

  Drop drops_[NUM_DROPS];
  uint8_t completion_pct_{0};
  uint32_t last_step_{0};
};

}  // namespace oraquadra
}  // namespace esphome
