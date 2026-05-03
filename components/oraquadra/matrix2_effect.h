#pragma once

// Matrix2: like Matrix-rain but drops only stop falling once every time-string
// LED has been hit. After completion the time stays solid for the rest of the
// minute. Same green raindrops, same trail, same capture-on-hit logic.

#include <cstdint>
#include <cstring>
#include "esp_random.h"
#include "effect.h"
#include "italian.h"

namespace esphome {
namespace oraquadra {

class Matrix2Effect final : public Effect {
 public:
  const char *name() const override { return "Matrix2"; }

  void on_minute_change(uint8_t /*h*/, uint8_t /*m*/) override {
    needs_reset_ = true;
    completed_ = false;
  }

  void update(uint32_t /*now_ms*/, Matrix &m, const ClockState &s) override {
    if (s.language == nullptr) return;

    if (needs_reset_ || s.hour != last_hour_ || s.minute != last_minute_) {
      std::memset(target_, 0, sizeof(target_));
      std::memset(active_, 0, sizeof(active_));
      mark_time_targets_(s);
      for (uint8_t i = 0; i < NUM_DROPS; i++) {
        init_drop_(drops_[i]);
      }
      last_hour_ = s.hour;
      last_minute_ = s.minute;
      completed_ = false;
      needs_reset_ = false;
    }

    if (completed_) {
      // Hold the time solid; mirror the original which also blinks seconds.
      s.language->render_time(m, s.hour, s.minute, s.color);
      return;
    }

    bool all_target_active = true;

    for (uint8_t i = 0; i < NUM_DROPS; i++) {
      Drop &drop = drops_[i];
      if (!drop.active) continue;

      int yInt = static_cast<int>(drop.y);
      if (yInt >= 0 && yInt < Matrix::HEIGHT) {
        uint16_t pos = Matrix::xy_to_led(drop.x, static_cast<uint8_t>(yInt));
        if (pos < Matrix::NUM_LEDS) {
          if (target_[pos] && !active_[pos]) {
            active_[pos] = true;
            m.set_pixel(pos, s.color);
          } else if (!target_[pos]) {
            uint8_t intensity = static_cast<uint8_t>(255 - (yInt * 16));
            m.set_pixel(pos, Color{0, intensity, 0});
            for (int trail = 1; trail <= TRAIL_LEN; trail++) {
              int trailY = yInt - trail;
              if (trailY >= 0) {
                uint16_t trailPos = Matrix::xy_to_led(drop.x, static_cast<uint8_t>(trailY));
                if (trailPos < Matrix::NUM_LEDS && !target_[trailPos]) {
                  uint8_t trail_int = static_cast<uint8_t>(intensity / (trail * 2));
                  m.set_pixel(trailPos, Color{0, trail_int, 0});
                }
              }
            }
          }
        }
      }

      drop.y += drop.speed;
      if (drop.y >= static_cast<float>(Matrix::HEIGHT)) {
        if (!completed_) {
          init_drop_(drop);
        } else {
          drop.active = false;
        }
      }
    }

    for (uint16_t i = 0; i < Matrix::NUM_LEDS; i++) {
      if (target_[i] && !active_[i]) {
        all_target_active = false;
        break;
      }
    }
    if (all_target_active && !completed_) {
      completed_ = true;
    }

    for (uint16_t i = 0; i < Matrix::NUM_LEDS; i++) {
      if (target_[i] && active_[i]) {
        m.set_pixel(i, s.color);
      }
    }
  }

 private:
  static constexpr uint8_t NUM_DROPS = 32;
  static constexpr uint8_t TRAIL_LEN = 13;
  static constexpr float BASE_SPEED = 0.15f;
  static constexpr float SPEED_VAR = 0.1f;
  static constexpr float START_Y_MIN = -3.0f;
  static constexpr float START_Y_MAX = 0.0f;

  struct Drop {
    uint8_t x{0};
    float y{0};
    float speed{0};
    bool active{false};
  };

  void init_drop_(Drop &drop) {
    drop.x = static_cast<uint8_t>(esp_random() % Matrix::WIDTH);
    float r = static_cast<float>(esp_random() & 0xFFFF) / 65536.0f;
    drop.y = START_Y_MIN + r * (START_Y_MAX - START_Y_MIN);
    float sr = static_cast<float>(esp_random() % 100) / 100.0f;
    drop.speed = BASE_SPEED + sr * SPEED_VAR;
    drop.active = true;
  }

  void mark_time_targets_(const ClockState &s) {
    mark_word_(it_words::PREFIX);
    mark_word_(it_words::HOURS[s.hour]);
    if (s.minute > 0) {
      mark_word_(it_words::AND_WORD);
      mark_minutes_(s.minute);
      mark_word_(it_words::SUFFIX);
    }
  }

  void mark_word_(const Word &w) {
    for (uint8_t i = 0; i < w.count; i++) {
      uint16_t led = w.leds[i];
      if (led < Matrix::NUM_LEDS) target_[led] = true;
    }
  }

  void mark_minutes_(uint8_t minute) {
    if (minute < 20) {
      mark_word_(it_words::MINUTE_UNITS[minute]);
      return;
    }
    uint8_t tens_idx = (minute / 10) - 2;
    uint8_t ones = minute % 10;
    bool elide = (ones == 1 || ones == 8);
    const auto &t = it_words::TENS[tens_idx];
    mark_word_(elide ? t.elided : t.full);
    if (ones > 0) mark_word_(it_words::MINUTE_UNITS[ones]);
  }

  void paint_seconds_(Matrix &m, const ClockState &s) {
    if (!s.blink_seconds) return;
    const int16_t hb = s.language->heartbeat_led();
    if (hb >= 0 && (s.second % 2) == 0) {
      m.set_pixel(static_cast<uint16_t>(hb), Color{0, 0, 0});
    }
  }

  Drop drops_[NUM_DROPS];
  bool target_[Matrix::NUM_LEDS]{};
  bool active_[Matrix::NUM_LEDS]{};
  bool needs_reset_{true};
  bool completed_{false};
  uint8_t last_hour_{255};
  uint8_t last_minute_{255};
};

}  // namespace oraquadra
}  // namespace esphome
