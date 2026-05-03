#pragma once

// =============================================================================
// Analog clock effect.
//
// Hands are drawn on the 60-LED cornice (perimeter ring):
//   - tick marks at 12/3/6/9 (faint)
//   - hour hand — dim amber, glides between hours via minute/12 sub-step
//   - minute hand — full-bright user color, exactly one LED
//   - seconds tick — very dim user color, one LED, only when blink_seconds
//
// The inner 14×14 stays dark by default. The IAQ frame (if enabled) is
// painted by the component before this effect runs, so its tint shows under
// the cornice positions we don't overwrite.
// =============================================================================

#include "effect.h"

namespace esphome {
namespace oraquadra {

class AnalogEffect final : public Effect {
 public:
  const char *name() const override { return "Analog"; }

  void update(uint32_t /*now_ms*/, Matrix &m, const ClockState &s) override {
    // 1) Tick marks at 12, 3, 6, 9.
    const Color tick_color = scale_(s.color, 24);
    for (uint8_t q = 0; q < 4; q++) {
      m.set_cornice(Matrix::minute_to_cornice(q * 15), tick_color);
    }

    // 2) Hour hand — amber, dim. Slides 1 sub-step every 12 minutes.
    const uint8_t h12       = s.hour % 12;
    const uint8_t hour_min  = static_cast<uint8_t>(h12 * 5 + s.minute / 12);
    const uint8_t hour_idx  = Matrix::minute_to_cornice(hour_min);
    m.set_cornice(hour_idx, Color{200, 100, 0});

    // 3) Minute hand — user color, full bright.
    m.set_cornice(Matrix::minute_to_cornice(s.minute), s.color);

    // 4) Seconds tick — only if blink-seconds is on.
    if (s.blink_seconds) {
      m.set_cornice(Matrix::minute_to_cornice(s.second), scale_(s.color, 64));
    }
  }

 private:
  static Color scale_(Color c, uint8_t scale_8) {
    return Color{
        static_cast<uint8_t>((c.r * scale_8) >> 8),
        static_cast<uint8_t>((c.g * scale_8) >> 8),
        static_cast<uint8_t>((c.b * scale_8) >> 8)};
  }
};

}  // namespace oraquadra
}  // namespace esphome
