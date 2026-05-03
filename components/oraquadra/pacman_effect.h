#pragma once

// Pacman: yellow Pacman walks across row 7 chasing 4 colored ghosts. Time
// text painted in user color in the rest of the matrix.

#include "effect.h"

namespace esphome {
namespace oraquadra {

class PacmanEffect final : public Effect {
 public:
  const char *name() const override { return "Pacman"; }

  void on_minute_change(uint8_t /*h*/, uint8_t /*m*/) override {
    pacman_x_ = -2;
    for (uint8_t i = 0; i < NUM_GHOSTS; i++) ghost_x_[i] = 4 + (i * 3);
  }

  void update(uint32_t now_ms, Matrix &m, const ClockState &s) override {
    if (now_ms - last_step_ < 200) {
      paint_(m, s);
      return;
    }
    last_step_ = now_ms;

    pacman_x_++;
    if (pacman_x_ >= Matrix::WIDTH + 4) pacman_x_ = -2;
    for (uint8_t i = 0; i < NUM_GHOSTS; i++) {
      ghost_x_[i]++;
      if (ghost_x_[i] >= Matrix::WIDTH + 8) ghost_x_[i] = -1 - (i * 2);
    }
    paint_(m, s);
  }

 private:
  static constexpr uint8_t NUM_GHOSTS = 4;
  static constexpr uint8_t ROW = 7;

  void paint_(Matrix &m, const ClockState &s) {
    if (s.language != nullptr) {
      s.language->render_time(m, s.hour, s.minute, s.color);
    }
    static const Color GHOST_COLORS[NUM_GHOSTS] = {
        Color{255, 0, 0},      // red (Blinky)
        Color{255, 180, 220},  // pink (Pinky)
        Color{0, 200, 255},    // cyan (Inky)
        Color{255, 165, 0},    // orange (Clyde)
    };
    for (uint8_t i = 0; i < NUM_GHOSTS; i++) {
      int8_t gx = ghost_x_[i];
      if (gx >= 0 && gx < (int8_t) Matrix::WIDTH) {
        m.set_pixel(static_cast<uint8_t>(gx), ROW, GHOST_COLORS[i]);
      }
    }
    if (pacman_x_ >= 0 && pacman_x_ < (int8_t) Matrix::WIDTH) {
      m.set_pixel(static_cast<uint8_t>(pacman_x_), ROW, Color{255, 220, 0});
    }
  }

  int8_t pacman_x_{-2};
  int8_t ghost_x_[NUM_GHOSTS]{4, 7, 10, 13};
  uint32_t last_step_{0};
};

}  // namespace oraquadra
}  // namespace esphome
