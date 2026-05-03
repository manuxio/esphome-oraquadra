#pragma once

// Matrix rain: 32 green raindrops fall down random columns; when a drop's
// head crosses an LED that belongs to the time string it "captures" that LED
// (painted in user colour from then on). Once every time-string pixel has
// been captured, the time stays solid until the minute rolls over.

#include <cstdint>
#include <cstring>
#include "esp_random.h"
#include "effect.h"
#include "italian.h"

namespace esphome {
namespace oraquadra {

class MatrixEffect final : public Effect {
 public:
  const char *name() const override { return "Matrix"; }

  void on_minute_change(uint8_t /*h*/, uint8_t /*m*/) override {
    needs_init_ = true;
  }

  void update(uint32_t /*now_ms*/, Matrix &m, const ClockState &s) override {
    if (s.language == nullptr) return;

    if (needs_init_ || s.hour != last_hour_ || s.minute != last_minute_) {
      std::memset(target_, 0, sizeof(target_));
      std::memset(active_, 0, sizeof(active_));
      mark_time_targets_(s);
      for (uint8_t i = 0; i < NUM_DROPS; i++) {
        init_drop_(drops_[i]);
      }
      last_hour_ = s.hour;
      last_minute_ = s.minute;
      needs_init_ = false;
    }

    // Walk every drop one frame.
    for (uint8_t i = 0; i < NUM_DROPS; i++) {
      Drop &drop = drops_[i];
      if (!drop.active) continue;

      int yInt = static_cast<int>(drop.y);

      if (yInt >= 0 && yInt < Matrix::HEIGHT) {
        uint16_t pos = Matrix::xy_to_led(drop.x, static_cast<uint8_t>(yInt));
        if (pos < Matrix::NUM_LEDS) {
          if (target_[pos] && !active_[pos]) {
            // Drop head hit a time-string LED — capture it in user colour.
            active_[pos] = true;
            m.set_pixel(pos, s.color);
          } else if (!target_[pos]) {
            // Free pixel — paint green head + fading trail.
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
        init_drop_(drop);
      }
    }

    // Already-captured target pixels stay lit even if no drop is on them this frame.
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
  static constexpr float SPEED_VAR = 0.01f;
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
    // y in [START_Y_MIN, START_Y_MAX)
    float r = static_cast<float>(esp_random() & 0xFFFF) / 65536.0f;  // [0,1)
    drop.y = START_Y_MIN + r * (START_Y_MAX - START_Y_MIN);
    float sr = static_cast<float>(esp_random() % 100) / 100.0f;
    drop.speed = BASE_SPEED + sr * SPEED_VAR;
    drop.active = true;
  }

  // Build target_[] by walking the same grammar as italian.h::render_time but
  // recording LED indices instead of painting them.
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

  Drop drops_[NUM_DROPS];
  bool target_[Matrix::NUM_LEDS]{};
  bool active_[Matrix::NUM_LEDS]{};
  bool needs_init_{true};
  uint8_t last_hour_{255};
  uint8_t last_minute_{255};
};

}  // namespace oraquadra
}  // namespace esphome
