#pragma once

// Moto: a single zigzag rider sweeps every cell of the matrix in
// boustrophedon order leaving a rainbow trail behind it. Time text painted
// on top in user color. Restart at minute change.

#include "effect.h"
#include "rainbow_effect.h"  // hsv_ helper duplicated; kept simple

namespace esphome {
namespace oraquadra {

class MotoEffect final : public Effect {
 public:
  const char *name() const override { return "Moto"; }

  void on_minute_change(uint8_t /*h*/, uint8_t /*m*/) override {
    pos_ = 0;
    hue_ = 0;
  }

  void update(uint32_t now_ms, Matrix &m, const ClockState &s) override {
    if (now_ms - last_step_ < 50) {
      paint_(m, s);
      return;
    }
    last_step_ = now_ms;
    if (pos_ < Matrix::NUM_LEDS) {
      // Compute serpentine cell.
      uint8_t y = pos_ / Matrix::WIDTH;
      uint8_t x = pos_ % Matrix::WIDTH;
      uint16_t led = Matrix::xy_to_led(x, y);
      // Color of trail at current hue.
      trail_[led] = ++hue_;
      pos_++;
    }
    // Trail decays slightly each step.
    for (uint16_t i = 0; i < Matrix::NUM_LEDS; i++) {
      if (trail_[i] > 0) trail_[i]--;
    }
    paint_(m, s);
  }

 private:
  void paint_(Matrix &m, const ClockState &s) {
    for (uint16_t i = 0; i < Matrix::NUM_LEDS; i++) {
      if (trail_[i] == 0) continue;
      Color c = hsv_(trail_[i]);
      // Scale by intensity proportional to trail freshness.
      uint8_t intensity = trail_[i];
      c.r = static_cast<uint8_t>((c.r * intensity) >> 8);
      c.g = static_cast<uint8_t>((c.g * intensity) >> 8);
      c.b = static_cast<uint8_t>((c.b * intensity) >> 8);
      m.set_pixel(i, c);
    }
    if (s.language != nullptr) {
      s.language->render_time(m, s.hour, s.minute, s.color);
    }
  }

  static Color hsv_(uint8_t h) {
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

  uint16_t pos_{0};
  uint8_t hue_{0};
  uint8_t trail_[Matrix::NUM_LEDS]{};
  uint32_t last_step_{0};
};

}  // namespace oraquadra
}  // namespace esphome
