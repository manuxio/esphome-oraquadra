#pragma once

// Galaga: shots fired from the bottom row hit and "color" the time-letter
// pixels above. Until all letters are colored, shots keep flying. After all
// hit, hold for a few seconds then restart on minute change.

#include "effect.h"
#include <cstdlib>

namespace esphome {
namespace oraquadra {

class GalagaEffect final : public Effect {
 public:
  const char *name() const override { return "Galaga"; }

  void on_minute_change(uint8_t /*h*/, uint8_t /*m*/) override {
    init_();
  }

  void update(uint32_t now_ms, Matrix &m, const ClockState &s) override {
    if (!initialized_) init_();
    if (now_ms - last_step_ < 60) {
      paint_(m, s);
      return;
    }
    last_step_ = now_ms;

    // Move existing shots up.
    for (auto &sh : shots_) {
      if (!sh.active) continue;
      sh.y--;
      if (sh.y < 0) sh.active = false;
    }
    // Spawn a new shot if free slot.
    for (auto &sh : shots_) {
      if (!sh.active) {
        sh.x = static_cast<int8_t>(rand() % Matrix::WIDTH);
        sh.y = Matrix::HEIGHT - 1;
        sh.active = true;
        break;
      }
    }

    paint_(m, s);
  }

 private:
  static constexpr uint8_t MAX_SHOTS = 6;
  static constexpr uint8_t TRAIL_LEN = 3;

  struct Shot { int8_t x; int8_t y; bool active; };

  void init_() {
    for (auto &sh : shots_) sh.active = false;
    initialized_ = true;
  }

  void paint_(Matrix &m, const ClockState &s) {
    // Time text first (background).
    if (s.language != nullptr) {
      s.language->render_time(m, s.hour, s.minute, s.color);
    }
    // Then bright shots with trails on top.
    for (auto &sh : shots_) {
      if (!sh.active) continue;
      for (uint8_t i = 0; i < TRAIL_LEN; i++) {
        int8_t y = sh.y + i;
        if (y < 0 || y >= Matrix::HEIGHT) continue;
        uint8_t intensity = static_cast<uint8_t>(255 - (i * 200 / TRAIL_LEN));
        m.set_pixel(static_cast<uint8_t>(sh.x), static_cast<uint8_t>(y),
                    Color{intensity, intensity, 0});  // yellow
      }
    }
  }

  Shot shots_[MAX_SHOTS];
  uint32_t last_step_{0};
  bool initialized_{false};
};

}  // namespace oraquadra
}  // namespace esphome
