#pragma once

// Drop: a single droplet at a slightly random center sends a ring out across
// the matrix. As the ring passes over time-string LEDs they latch into the
// chosen wave color. After the ring leaves the matrix, the time stays solid.

#include <cmath>
#include <cstdint>
#include <cstring>
#include "esp_random.h"
#include "effect.h"
#include "italian.h"

namespace esphome {
namespace oraquadra {

class DropEffect final : public Effect {
 public:
  const char *name() const override { return "Drop"; }

  void on_minute_change(uint8_t /*h*/, uint8_t /*m*/) override {
    active_ = false;
    completed_ = false;
    radius_ = 0.0f;
    wave_count_ = 0;
  }

  void update(uint32_t now_ms, Matrix &m, const ClockState &s) override {
    if (s.language == nullptr) return;

    if (completed_) {
      // Hold final state in waveColor.
      m.paint_word(it_words::PREFIX, wave_color_);
      m.paint_word(it_words::HOURS[s.hour], wave_color_);
      if (s.minute > 0) {
        m.paint_word(it_words::AND_WORD, wave_color_);
        s.language->render_phase(m, s.hour, s.minute, wave_color_, 3);
        m.paint_word(it_words::SUFFIX, wave_color_);
      }
      return;
    }

    if (!active_) {
      active_ = true;
      radius_ = 0.0f;
      center_x_ = static_cast<uint8_t>(7 + (esp_random() % 3));
      center_y_ = static_cast<uint8_t>(7 + (esp_random() % 3));
      start_ms_ = now_ms;
      last_update_ = now_ms;
      // Wave + captured letters use the user's foreground color.
      wave_color_ = s.color;
      wave_count_ = 0;
      std::memset(target_, 0, sizeof(target_));
      mark_time_targets_(s);
    }

    if (now_ms - last_update_ < UPDATE_INTERVAL_MS) {
      paint_frame_(m);
      return;
    }
    last_update_ = now_ms;

    radius_ += WAVE_SPEED;
    if (radius_ > MAX_RADIUS) {
      completed_ = true;
      return;
    }

    paint_frame_(m);
  }

 private:
  static constexpr uint32_t UPDATE_INTERVAL_MS = 60;
  static constexpr float MAX_RADIUS = 24.0f;
  static constexpr float WAVE_SPEED = 0.4f;

  void paint_frame_(Matrix &m) {
    float cx = static_cast<float>(center_x_);
    float cy = static_cast<float>(center_y_);
    constexpr float wave_width = 2.5f;

    for (uint8_t y = 0; y < Matrix::HEIGHT; y++) {
      for (uint8_t x = 0; x < Matrix::WIDTH; x++) {
        float dx = static_cast<float>(x) - cx;
        float dy = static_cast<float>(y) - cy;
        float dist = std::sqrt(dx * dx + dy * dy);
        uint16_t pos = Matrix::xy_to_led(x, y);
        if (pos >= Matrix::NUM_LEDS) continue;

        float wave_dist = std::fabs(dist - radius_);

        if (wave_dist < wave_width) {
          uint8_t intensity = static_cast<uint8_t>(255.0f - wave_dist * 100.0f);
          if (target_[pos]) {
            m.set_pixel(pos, scale_(wave_color_, intensity));
          } else {
            // Water trail in the same foreground color, dimmer than the wave
            // crest so the captured time letters stand out.
            m.set_pixel(pos, scale_(wave_color_, static_cast<uint8_t>(intensity / 2)));
          }
        }

        if (target_[pos] && dist < radius_ - wave_width) {
          int calc = static_cast<int>((radius_ - dist) * 20.0f);
          if (calc > 255) calc = 255;
          uint8_t reveal = static_cast<uint8_t>(calc);
          m.set_pixel(pos, scale_(wave_color_, reveal));
        }
      }
    }

    if (radius_ < 3.0f) {
      uint16_t cpos = Matrix::xy_to_led(center_x_, center_y_);
      if (cpos < Matrix::NUM_LEDS) m.set_pixel(cpos, Color{255, 255, 255});
    }
  }

  void paint_seconds_(Matrix &m, const ClockState &s) {
    if (!s.blink_seconds) return;
    const int16_t hb = s.language->heartbeat_led();
    if (hb >= 0 && (s.second % 2) == 0) {
      m.set_pixel(static_cast<uint16_t>(hb), Color{0, 0, 0});
    }
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

  static Color scale_(Color c, uint8_t s_) {
    return Color{
        static_cast<uint8_t>((c.r * s_) >> 8),
        static_cast<uint8_t>((c.g * s_) >> 8),
        static_cast<uint8_t>((c.b * s_) >> 8),
    };
  }

  // Standard 6-sector HSV→RGB. h, s, v all uint8_t (h=0..255 mapping to 0..360).
  static Color hsv_to_rgb_(uint8_t h, uint8_t s, uint8_t v) {
    if (s == 0) return Color{v, v, v};
    uint8_t region = h / 43;          // 0..5
    uint8_t remainder = (h - region * 43) * 6;
    uint8_t p = static_cast<uint8_t>((v * (255 - s)) >> 8);
    uint8_t q = static_cast<uint8_t>((v * (255 - ((s * remainder) >> 8))) >> 8);
    uint8_t t = static_cast<uint8_t>((v * (255 - ((s * (255 - remainder)) >> 8))) >> 8);
    switch (region) {
      case 0: return Color{v, t, p};
      case 1: return Color{q, v, p};
      case 2: return Color{p, v, t};
      case 3: return Color{p, q, v};
      case 4: return Color{t, p, v};
      default: return Color{v, p, q};
    }
  }

  bool active_{false};
  bool completed_{false};
  float radius_{0.0f};
  uint8_t center_x_{8};
  uint8_t center_y_{8};
  uint32_t last_update_{0};
  uint32_t start_ms_{0};
  Color wave_color_{0, 0, 255};
  uint8_t wave_count_{0};
  bool target_[Matrix::NUM_LEDS]{};
};

}  // namespace oraquadra
}  // namespace esphome
