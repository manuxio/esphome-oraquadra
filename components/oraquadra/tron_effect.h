#pragma once

// Tron: 3 light bikes ride around the matrix leaving fading trails. They
// random-walk with no collision detection (simpler than the original .ino).
// Time text painted on top.

#include "effect.h"
#include <cstdlib>

namespace esphome {
namespace oraquadra {

class TronEffect final : public Effect {
 public:
  const char *name() const override { return "Tron"; }

  void update(uint32_t now_ms, Matrix &m, const ClockState &s) override {
    if (now_ms - last_step_ < 100) {
      paint_(m, s);
      return;
    }
    last_step_ = now_ms;

    if (!initialized_) init_();

    // Decay all trail intensities.
    for (uint16_t i = 0; i < Matrix::NUM_LEDS; i++) {
      if (trail_[i] > 12) trail_[i] -= 12;
      else trail_[i] = 0;
    }

    // Move each bike one step.
    for (auto &b : bikes_) {
      // 30% chance to turn 90° randomly.
      if ((rand() % 10) < 3) {
        bool turn_right = (rand() % 2) == 0;
        b.dir = static_cast<uint8_t>((b.dir + (turn_right ? 1 : 3)) % 4);
      }
      switch (b.dir) {
        case 0: b.y--; break;       // up
        case 1: b.x++; break;       // right
        case 2: b.y++; break;       // down
        case 3: b.x--; break;       // left
      }
      // Wrap around.
      if (b.x < 0) b.x = Matrix::WIDTH - 1;
      if (b.x >= (int8_t) Matrix::WIDTH) b.x = 0;
      if (b.y < 0) b.y = Matrix::HEIGHT - 1;
      if (b.y >= (int8_t) Matrix::HEIGHT) b.y = 0;
      // Mark trail at full.
      trail_[Matrix::xy_to_led(static_cast<uint8_t>(b.x), static_cast<uint8_t>(b.y))] = 255;
    }

    paint_(m, s);
  }

 private:
  static constexpr uint8_t NUM_BIKES = 3;
  struct Bike { int8_t x; int8_t y; uint8_t dir; Color color; };

  void init_() {
    bikes_[0] = {0, 0, 1, Color{0, 200, 255}};       // cyan
    bikes_[1] = {15, 0, 3, Color{255, 80, 0}};       // orange
    bikes_[2] = {7, 15, 0, Color{180, 0, 255}};      // violet
    initialized_ = true;
  }

  void paint_(Matrix &m, const ClockState &s) {
    // Render each pixel based on trail intensity. Use bike colors mixed by
    // simply assigning the pixel color to whichever bike most recently
    // touched it — approximate by using avg of bike colors weighted by who's
    // closest (cheap approximation: use bike[0] for now).
    for (uint16_t i = 0; i < Matrix::NUM_LEDS; i++) {
      if (trail_[i] == 0) continue;
      // Use bike 0's color as base — could be smarter but keeps it simple.
      uint8_t intensity = trail_[i];
      Color c{
          static_cast<uint8_t>((bikes_[0].color.r * intensity) >> 8),
          static_cast<uint8_t>((bikes_[0].color.g * intensity) >> 8),
          static_cast<uint8_t>((bikes_[0].color.b * intensity) >> 8),
      };
      m.set_pixel(i, c);
    }
    if (s.language != nullptr) {
      s.language->render_time(m, s.hour, s.minute, s.color);
    }
  }

  Bike bikes_[NUM_BIKES];
  uint8_t trail_[Matrix::NUM_LEDS]{};
  uint32_t last_step_{0};
  bool initialized_{false};
};

}  // namespace oraquadra
}  // namespace esphome
