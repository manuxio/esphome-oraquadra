#pragma once

// Pacman: a 2-pixel yellow Pacman head leads four coloured ghosts across the
// linear LED index space. As the procession crosses each time-string pixel
// that pixel latches on in a random text colour. Once Pacman walks off the
// end of the strip, the time stays solid until the minute rolls over.

#include <cstdint>
#include <cstring>
#include "esp_random.h"
#include "effect.h"
#include "italian.h"

namespace esphome {
namespace oraquadra {

class PacmanEffect final : public Effect {
 public:
  const char *name() const override { return "Pacman"; }

  void on_minute_change(uint8_t /*h*/, uint8_t /*m*/) override {
    needs_init_ = true;
  }

  void update(uint32_t now_ms, Matrix &m, const ClockState &s) override {
    if (s.language == nullptr) return;

    if (needs_init_ || s.minute != last_minute_) {
      init_(s);
      last_minute_ = s.minute;
      needs_init_ = false;
      return;
    }

    if (completion_time_ != 0) {
      // Stay on captured pixels.
      for (uint16_t i = 0; i < Matrix::NUM_LEDS; i++) {
        if (text_pixels_[i]) m.set_pixel(i, text_color_);
      }
      return;
    }

    if (now_ms - last_update_ < SPEED_MS) {
      paint_frame_(m);
      return;
    }
    last_update_ = now_ms;

    pacman_pos_++;
    for (uint8_t i = 0; i < NUM_GHOSTS; i++) ghost_pos_[i]++;

    // Pacman head: 2 pixels.
    if (pacman_pos_ >= 0 && pacman_pos_ < static_cast<int16_t>(Matrix::NUM_LEDS)) {
      int16_t left = pacman_pos_;
      int16_t right = pacman_pos_ + 1;
      if (left >= 0 && left < static_cast<int16_t>(Matrix::NUM_LEDS) && target_[left]) {
        text_pixels_[left] = true;
      }
      if (right >= 0 && right < static_cast<int16_t>(Matrix::NUM_LEDS) && target_[right]) {
        text_pixels_[right] = true;
      }
    }
    for (uint8_t i = 0; i < NUM_GHOSTS; i++) {
      int16_t gp = ghost_pos_[i];
      if (gp >= 0 && gp < static_cast<int16_t>(Matrix::NUM_LEDS)) {
        if (target_[gp]) text_pixels_[gp] = true;
      }
    }

    paint_frame_(m);

    if (pacman_pos_ >= static_cast<int16_t>(Matrix::NUM_LEDS) && completion_time_ == 0) {
      completion_time_ = now_ms;
    }
  }

 private:
  static constexpr uint8_t NUM_GHOSTS = 4;
  static constexpr uint32_t SPEED_MS = 50;

  void init_(const ClockState &s) {
    completion_time_ = 0;
    text_hue_ = static_cast<uint8_t>(esp_random() & 0xFF);
    text_color_ = hsv_(text_hue_, 255, 255);
    std::memset(text_pixels_, 0, sizeof(text_pixels_));

    pacman_pos_ = -2;
    for (uint8_t i = 0; i < NUM_GHOSTS; i++) {
      ghost_pos_[i] = pacman_pos_ - 4 - (i * 3);
    }

    std::memset(target_, 0, sizeof(target_));
    mark_word_(it_words::PREFIX);
    mark_word_(it_words::HOURS[s.hour]);
    if (s.minute > 0) {
      mark_word_(it_words::AND_WORD);
      mark_minutes_(s.minute);
      mark_word_(it_words::SUFFIX);
    }
    last_update_ = 0;
  }

  void paint_frame_(Matrix &m) {
    // Pacman head.
    if (pacman_pos_ >= 0 && pacman_pos_ < static_cast<int16_t>(Matrix::NUM_LEDS)) {
      int16_t left = pacman_pos_;
      int16_t right = pacman_pos_ + 1;
      if (left >= 0 && left < static_cast<int16_t>(Matrix::NUM_LEDS)) {
        m.set_pixel(static_cast<uint16_t>(left), Color{255, 255, 0});
      }
      if (right >= 0 && right < static_cast<int16_t>(Matrix::NUM_LEDS)) {
        m.set_pixel(static_cast<uint16_t>(right), Color{255, 255, 0});
      }
    }

    // Ghosts.
    static const Color GHOST_COLORS[NUM_GHOSTS] = {
        Color{255, 0, 0},      // red
        Color{255, 182, 255},  // pink
        Color{0, 255, 255},    // cyan
        Color{255, 165, 0},    // orange
    };
    for (uint8_t i = 0; i < NUM_GHOSTS; i++) {
      int16_t gp = ghost_pos_[i];
      if (gp >= 0 && gp < static_cast<int16_t>(Matrix::NUM_LEDS)) {
        m.set_pixel(static_cast<uint16_t>(gp), GHOST_COLORS[i]);
      }
    }

    // Captured text pixels (drawn last so they show on top).
    for (uint16_t i = 0; i < Matrix::NUM_LEDS; i++) {
      if (text_pixels_[i]) m.set_pixel(i, text_color_);
    }
  }

  void paint_seconds_(Matrix &m, const ClockState &s) {
    if (!s.blink_seconds) return;
    const int16_t hb = s.language->heartbeat_led();
    if (hb >= 0 && (s.second % 2) == 0) {
      m.set_pixel(static_cast<uint16_t>(hb), Color{0, 0, 0});
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

  static Color hsv_(uint8_t h, uint8_t s, uint8_t v) {
    if (s == 0) return Color{v, v, v};
    uint8_t region = h / 43;
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

  bool needs_init_{true};
  uint32_t last_update_{0};
  uint32_t completion_time_{0};
  int16_t pacman_pos_{-2};
  int16_t ghost_pos_[NUM_GHOSTS]{};
  uint8_t text_hue_{0};
  Color text_color_{255, 255, 255};
  bool text_pixels_[Matrix::NUM_LEDS]{};
  bool target_[Matrix::NUM_LEDS]{};
  uint8_t last_minute_{255};
};

}  // namespace oraquadra
}  // namespace esphome
