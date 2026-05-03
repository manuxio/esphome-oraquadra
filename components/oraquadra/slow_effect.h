#pragma once

// Slow: a single global fade-in of the entire time string from 0 to full
// brightness over ~30 steps of 70ms each (~2.1s), then holds for 500ms before
// settling into a solid display until the minute changes.

#include <cstdint>
#include "esp_random.h"
#include "effect.h"
#include "italian.h"

namespace esphome {
namespace oraquadra {

class SlowEffect final : public Effect {
 public:
  const char *name() const override { return "Slow"; }

  void on_minute_change(uint8_t /*h*/, uint8_t /*m*/) override {
    active_ = false;
    completed_ = false;
    hold_phase_ = false;
    step_ = 0;
    last_step_ms_ = 0;
    hold_start_ms_ = 0;
  }

  void update(uint32_t now_ms, Matrix &m, const ClockState &s) override {
    if (s.language == nullptr) return;

    if (completed_) {
      paint_full_(m, s);
      return;
    }

    if (!active_) {
      active_ = true;
      step_ = 0;
      target_brightness_ = 255;
      increment_ = target_brightness_ / 30;
      if (increment_ == 0) increment_ = 1;
      last_step_ms_ = now_ms;
      hold_phase_ = false;

      // The original honoured a `randomColor` toggle that recoloured every
      // chunk independently. The ESPHome port has no such toggle yet, so we
      // mirror the disabled branch and use the user's chosen color uniformly.
      sono_color_ = s.color;
      hour_color_ = s.color;
      e_color_ = s.color;
      minutes_color_ = s.color;
      minuti_color_ = s.color;
    }

    if (hold_phase_) {
      paint_full_(m, s);
      if (now_ms - hold_start_ms_ >= 500) {
        completed_ = true;
      }
      return;
    }

    if (now_ms - last_step_ms_ < 70) {
      paint_fade_(m, s);
      return;
    }
    last_step_ms_ = now_ms;

    paint_fade_(m, s);
    step_++;

    uint16_t current_level = static_cast<uint16_t>(step_) * increment_;
    if (current_level >= target_brightness_) {
      hold_phase_ = true;
      hold_start_ms_ = now_ms;
    }
  }

 private:
  uint8_t current_brightness_level_() const {
    uint16_t lvl = static_cast<uint16_t>(step_) * increment_;
    if (lvl > target_brightness_) lvl = target_brightness_;
    return static_cast<uint8_t>(lvl);
  }

  static Color scale_(Color c, uint8_t s_) {
    return Color{
        static_cast<uint8_t>((c.r * s_) >> 8),
        static_cast<uint8_t>((c.g * s_) >> 8),
        static_cast<uint8_t>((c.b * s_) >> 8),
    };
  }

  void paint_fade_(Matrix &m, const ClockState &s) {
    uint8_t lvl = current_brightness_level_();
    uint8_t scale = (target_brightness_ == 0)
                        ? 0
                        : static_cast<uint8_t>((static_cast<uint16_t>(lvl) * 255) / target_brightness_);

    m.paint_word(it_words::PREFIX, scale_(sono_color_, scale));
    m.paint_word(it_words::HOURS[s.hour], scale_(hour_color_, scale));
    if (s.minute > 0) {
      m.paint_word(it_words::AND_WORD, scale_(e_color_, scale));
      s.language->render_phase(m, s.hour, s.minute, scale_(minutes_color_, scale), 3);
      m.paint_word(it_words::SUFFIX, scale_(minuti_color_, scale));
    }
  }

  void paint_full_(Matrix &m, const ClockState &s) {
    m.paint_word(it_words::PREFIX, sono_color_);
    m.paint_word(it_words::HOURS[s.hour], hour_color_);
    if (s.minute > 0) {
      m.paint_word(it_words::AND_WORD, e_color_);
      s.language->render_phase(m, s.hour, s.minute, minutes_color_, 3);
      m.paint_word(it_words::SUFFIX, minuti_color_);
    }
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
  bool hold_phase_{false};
  uint8_t step_{0};
  uint8_t target_brightness_{255};
  uint8_t increment_{1};
  uint32_t last_step_ms_{0};
  uint32_t hold_start_ms_{0};
  Color sono_color_{255, 255, 255};
  Color hour_color_{255, 255, 255};
  Color e_color_{255, 255, 255};
  Color minutes_color_{255, 255, 255};
  Color minuti_color_{255, 255, 255};
};

}  // namespace oraquadra
}  // namespace esphome
