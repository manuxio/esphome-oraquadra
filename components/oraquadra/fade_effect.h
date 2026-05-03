#pragma once

// Fade: like Words but on minute change the text fades out then back in over
// ~2 seconds. Visual rhythm at every minute boundary.

#include "effect.h"

namespace esphome {
namespace oraquadra {

class FadeEffect final : public Effect {
 public:
  const char *name() const override { return "Fade"; }

  void on_minute_change(uint8_t /*h*/, uint8_t /*m*/) override {
    fade_start_ms_ = millis();
  }

  void update(uint32_t now_ms, Matrix &m, const ClockState &s) override {
    if (s.language == nullptr) return;

    constexpr uint32_t FADE_MS = 2000;
    uint8_t scale = 255;
    if (fade_start_ms_ != 0 && now_ms - fade_start_ms_ < FADE_MS) {
      // Triangular wave: 0→255 in first half, 255→0 in second half.
      // Wait — we want fade-out then fade-in, so: 255→0→255. Use a /\ shape
      // with min in the middle.
      uint32_t t = now_ms - fade_start_ms_;
      uint32_t half = FADE_MS / 2;
      if (t < half) {
        scale = static_cast<uint8_t>(255 - (t * 255 / half));  // 255 → 0
      } else {
        scale = static_cast<uint8_t>(((t - half) * 255) / half);  // 0 → 255
      }
    }

    Color c{
        static_cast<uint8_t>((s.color.r * scale) >> 8),
        static_cast<uint8_t>((s.color.g * scale) >> 8),
        static_cast<uint8_t>((s.color.b * scale) >> 8),
    };
    s.language->render_time(m, s.hour, s.minute, c);
  }

 private:
  uint32_t fade_start_ms_{0};
};

}  // namespace oraquadra
}  // namespace esphome
