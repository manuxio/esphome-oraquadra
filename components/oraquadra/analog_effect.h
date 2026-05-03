#pragma once

// Analog clock effect — real hands rasterized as lines from the matrix
// center toward the target angle. Like a wall clock: 12 at top, clockwise.
//
//   Minute hand → reaches the matrix edge (length 7.5)
//   Hour hand   → stops short (length 5)
//   Seconds hand → length 6.5, dim, only if Blink Seconds is ON
//
// Tick marks at 12/3/6/9 stay on the perimeter so the user can read the
// angle even when no hand is near them.

#include "effect.h"
#include <cmath>

namespace esphome {
namespace oraquadra {

class AnalogEffect final : public Effect {
 public:
  const char *name() const override { return "Analog"; }

  void update(uint32_t /*now_ms*/, Matrix &m, const ClockState &s) override {
    // Tick marks at the perimeter (faint, ~12% of user color).
    const Color tick = scale_(s.color, 32);
    m.set_pixel(8,  0, tick);   // 12
    m.set_pixel(15, 8, tick);   // 3
    m.set_pixel(8, 15, tick);   // 6
    m.set_pixel(0,  8, tick);   // 9

    // Draw order: minute first, then hour on top, then seconds on top of all
    // — so the long minute hand is visible past the hour tip, the hour hand
    // overlays its shorter base, and seconds is always crisp.
    const float minute_angle = s.minute * (TWO_PI_ / 60.0f);
    draw_hand_(m, minute_angle, 7.5f, s.color);

    const float hour_angle =
        ((s.hour % 12) + s.minute / 60.0f) * (TWO_PI_ / 12.0f);
    draw_hand_(m, hour_angle, 5.0f, Color{200, 100, 0});  // dim amber

    if (s.blink_seconds) {
      const float second_angle = s.second * (TWO_PI_ / 60.0f);
      draw_hand_(m, second_angle, 6.5f, scale_(s.color, 90));
    }
  }

 private:
  static constexpr float TWO_PI_ = 6.28318530717958f;

  static Color scale_(Color c, uint8_t scale_8) {
    return Color{
        static_cast<uint8_t>((c.r * scale_8) >> 8),
        static_cast<uint8_t>((c.g * scale_8) >> 8),
        static_cast<uint8_t>((c.b * scale_8) >> 8)};
  }

  // Rasterize a line from the matrix center toward `length` cells in the
  // given clock-face angle. 0 = up (12 o'clock), positive = clockwise.
  static void draw_hand_(Matrix &m, float angle, float length, Color c) {
    constexpr float CX = 7.5f;       // center x (between cells 7 and 8)
    constexpr float CY = 7.5f;       // center y
    const float dx = std::sin(angle);
    const float dy = -std::cos(angle);   // up is -y on the matrix
    // Sub-pixel stepping: ~2.5 samples per cell guarantees no gaps even on
    // diagonals.
    const int steps = static_cast<int>(length * 2.5f) + 1;
    for (int i = 0; i <= steps; i++) {
      const float r = length * (static_cast<float>(i) / steps);
      const int px = static_cast<int>(CX + dx * r + 0.5f);
      const int py = static_cast<int>(CY + dy * r + 0.5f);
      if (px >= 0 && px < 16 && py >= 0 && py < 16) {
        m.set_pixel(static_cast<uint8_t>(px), static_cast<uint8_t>(py), c);
      }
    }
  }
};

}  // namespace oraquadra
}  // namespace esphome
