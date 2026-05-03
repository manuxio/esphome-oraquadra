#pragma once

// Fade: reveals each grammatical chunk in turn (SONO LE -> hour -> E -> minutes
// -> MINUTI), each fading from 0 to full brightness over ~2.5s. Once the last
// chunk has resolved the time stays solid until the minute rolls over.

#include <cstdint>
#include "effect.h"
#include "italian.h"

namespace esphome {
namespace oraquadra {

class FadeEffect final : public Effect {
 public:
  const char *name() const override { return "Fade"; }

  void on_minute_change(uint8_t /*h*/, uint8_t /*m*/) override {
    active_ = false;
    completed_ = false;
    phase_ = 0;
    step_ = 0;
    last_step_ms_ = 0;
  }

  void update(uint32_t now_ms, Matrix &m, const ClockState &s) override {
    if (s.language == nullptr) return;

    // Once finished, hold the solid time and blink the heartbeat LED.
    if (completed_) {
      paint_full_(m, s, s.color);
      return;
    }

    if (!active_) {
      active_ = true;
      phase_ = 0;
      step_ = 0;
      last_step_ms_ = now_ms;
    }

    // 50ms between steps → STEPS * 50 = 2.5s per phase.
    if (now_ms - last_step_ms_ < 50) {
      // Re-paint last frame so we don't flicker against the once-per-frame
      // clear performed by the component.
      paint_progress_(m, s, current_fade_color_(s));
      return;
    }
    last_step_ms_ = now_ms;

    paint_progress_(m, s, current_fade_color_(s));

    // Advance step / phase.
    step_++;
    if (step_ >= STEPS) {
      step_ = 0;
      phase_++;

      // When mm == 0 there is no AND/MINUTES/SUFFIX phase — jump to done.
      if (s.minute == 0 && phase_ == 2) {
        phase_ = 5;
      }
      if (phase_ > 4) {
        completed_ = true;
      }
    }
  }

 private:
  static constexpr uint8_t STEPS = 50;

  Color current_fade_color_(const ClockState &s) const {
    // map(step, 0, STEPS-1, 0, 255)
    uint16_t b = (static_cast<uint16_t>(step_) * 255) / (STEPS - 1);
    if (b > 255) b = 255;
    return Color{
        static_cast<uint8_t>((s.color.r * b) >> 8),
        static_cast<uint8_t>((s.color.g * b) >> 8),
        static_cast<uint8_t>((s.color.b * b) >> 8),
    };
  }

  // Paint already-completed phases at full color, plus the in-progress phase
  // at the current fade level.
  void paint_progress_(Matrix &m, const ClockState &s, Color fade) {
    // Completed phases (full brightness)
    if (phase_ > 0) m.paint_word(it_words::PREFIX, s.color);
    if (phase_ > 1) m.paint_word(it_words::HOURS[s.hour], s.color);
    if (phase_ > 2 && s.minute > 0) m.paint_word(it_words::AND_WORD, s.color);
    if (phase_ > 3 && s.minute > 0) s.language->render_phase(m, s.hour, s.minute, s.color, 3);
    if (phase_ > 4 && s.minute > 0) m.paint_word(it_words::SUFFIX, s.color);

    // In-progress phase at fade level.
    switch (phase_) {
      case 0:
        m.paint_word(it_words::PREFIX, fade);
        break;
      case 1:
        m.paint_word(it_words::PREFIX, s.color);
        m.paint_word(it_words::HOURS[s.hour], fade);
        break;
      case 2:
        if (s.minute > 0) m.paint_word(it_words::AND_WORD, fade);
        break;
      case 3:
        if (s.minute > 0) s.language->render_phase(m, s.hour, s.minute, fade, 3);
        break;
      case 4:
        if (s.minute > 0) m.paint_word(it_words::SUFFIX, fade);
        break;
    }
  }

  void paint_full_(Matrix &m, const ClockState &s, Color c) {
    s.language->render_time(m, s.hour, s.minute, c);
  }

  void paint_seconds_(Matrix &m, const ClockState &s) {
    if (!s.blink_seconds) return;
    const int16_t hb = s.language->heartbeat_led();
    if (hb >= 0 && (s.second % 2) == 0) {
      m.set_pixel(static_cast<uint16_t>(hb), Color{0, 0, 0});
    }
  }

  bool active_{false};
  bool completed_{false};
  uint8_t phase_{0};
  uint8_t step_{0};
  uint32_t last_step_ms_{0};
};

}  // namespace oraquadra
}  // namespace esphome
