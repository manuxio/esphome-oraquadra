#pragma once

// Moto: a single bike starts in a random corner and zigzags row-by-row across
// the matrix in user color, dragging an 8-pixel rainbow tail. As it crosses
// time-string LEDs they latch in the random text color. Once the bike walks
// off the matrix, the time stays solid until the minute rolls over.

#include <cstdint>
#include <cstring>
#include "esp_random.h"
#include "effect.h"
#include "italian.h"

namespace esphome {
namespace oraquadra {

class MotoEffect final : public Effect {
 public:
  const char *name() const override { return "Moto"; }

  void on_minute_change(uint8_t /*h*/, uint8_t /*m*/) override {
    needs_init_ = true;
  }

  void update(uint32_t now_ms, Matrix &m, const ClockState &s) override {
    if (s.language == nullptr) return;

    if (s.minute == 0) {
      // Original turns off LED 116 (the 'E') when minutes==0.
      text_pixels_[116] = false;
    }

    if (needs_init_ || s.minute != last_minute_) {
      init_(s);
      last_minute_ = s.minute;
      last_hour_ = s.hour;
      needs_init_ = false;
      return;
    }

    if (completed_) {
      if (last_hour_ != s.hour || last_minute_ != s.minute) {
        init_(s);
        last_hour_ = s.hour;
        last_minute_ = s.minute;
        return;
      }
      for (uint16_t i = 0; i < Matrix::NUM_LEDS; i++) {
        if (text_pixels_[i]) m.set_pixel(i, text_color_);
      }
      return;
    }

    if (now_ms - last_update_ < SPEED_MS) {
      // Repaint last frame.
      paint_frame_(m);
      return;
    }
    last_update_ = now_ms;

    // Shift trail colors and update head color.
    for (int i = TRAIL_LEN - 1; i > 0; i--) {
      trail_colors_[i] = trail_colors_[i - 1];
    }
    trail_colors_[0] = hsv_(trail_hue_, 255, 255);
    trail_hue_ += 8;

    // Push current position into trail.
    for (int i = TRAIL_LEN - 1; i > 0; i--) {
      trail_x_[i] = trail_x_[i - 1];
      trail_y_[i] = trail_y_[i - 1];
    }
    trail_x_[0] = static_cast<uint16_t>(moto_x_);
    trail_y_[0] = static_cast<uint16_t>(moto_y_);
    if (trail_len_ < TRAIL_LEN) trail_len_++;

    // Capture target pixel under bike.
    uint16_t cur_pos = Matrix::xy_to_led(static_cast<uint8_t>(moto_x_), static_cast<uint8_t>(moto_y_));
    if (cur_pos < Matrix::NUM_LEDS && target_[cur_pos]) {
      text_pixels_[cur_pos] = true;
    }

    paint_frame_(m);

    // Move bike.
    moto_pos_++;
    int8_t nextX = moto_x_ + dir_x_;
    if (nextX < 0 || nextX >= static_cast<int8_t>(Matrix::WIDTH)) {
      dir_x_ = -dir_x_;
      moto_y_ += dir_y_;
      if (moto_y_ < 0 || moto_y_ >= static_cast<int8_t>(Matrix::HEIGHT)) {
        completed_ = true;
        return;
      }
    } else {
      moto_x_ = nextX;
    }
  }

 private:
  static constexpr uint32_t SPEED_MS = 30;
  static constexpr uint8_t TRAIL_LEN = 8;

  void init_(const ClockState &s) {
    moto_pos_ = 0;
    completed_ = false;
    trail_hue_ = 0;
    text_hue_ = static_cast<uint8_t>(esp_random() & 0xFF);
    text_color_ = hsv_(text_hue_, 255, 255);
    trail_len_ = 0;
    last_update_ = 0;

    uint8_t corner = static_cast<uint8_t>(esp_random() % 4);
    switch (corner) {
      case 0:
        moto_x_ = 0;
        moto_y_ = 0;
        dir_x_ = 1;
        dir_y_ = 1;
        break;
      case 1:
        moto_x_ = Matrix::WIDTH - 1;
        moto_y_ = 0;
        dir_x_ = -1;
        dir_y_ = 1;
        break;
      case 2:
        moto_x_ = 0;
        moto_y_ = Matrix::HEIGHT - 1;
        dir_x_ = 1;
        dir_y_ = -1;
        break;
      case 3:
        moto_x_ = Matrix::WIDTH - 1;
        moto_y_ = Matrix::HEIGHT - 1;
        dir_x_ = -1;
        dir_y_ = -1;
        break;
    }

    std::memset(text_pixels_, 0, sizeof(text_pixels_));
    for (uint8_t i = 0; i < TRAIL_LEN; i++) {
      trail_colors_[i] = Color{0, 0, 0};
      trail_x_[i] = 0;
      trail_y_[i] = 0;
    }

    std::memset(target_, 0, sizeof(target_));
    mark_word_(it_words::PREFIX);
    mark_word_(it_words::HOURS[s.hour]);
    if (s.minute > 0) {
      mark_word_(it_words::AND_WORD);
      mark_minutes_(s.minute);
      mark_word_(it_words::SUFFIX);
    }
  }

  void paint_frame_(Matrix &m) {
    // Trail.
    for (uint8_t i = 0; i < trail_len_; i++) {
      uint16_t pos = Matrix::xy_to_led(static_cast<uint8_t>(trail_x_[i]),
                                       static_cast<uint8_t>(trail_y_[i]));
      if (pos < Matrix::NUM_LEDS) {
        uint8_t intensity = static_cast<uint8_t>(255 - (i * (255 / TRAIL_LEN)));
        Color c = trail_colors_[i];
        m.set_pixel(pos, Color{
            static_cast<uint8_t>((c.r * intensity) >> 8),
            static_cast<uint8_t>((c.g * intensity) >> 8),
            static_cast<uint8_t>((c.b * intensity) >> 8),
        });
      }
    }
    // Captured time pixels.
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
  bool completed_{false};
  int8_t moto_x_{0};
  int8_t moto_y_{0};
  int8_t dir_x_{1};
  int8_t dir_y_{1};
  uint16_t moto_pos_{0};
  uint8_t trail_hue_{0};
  uint8_t text_hue_{0};
  Color text_color_{255, 255, 255};
  Color trail_colors_[TRAIL_LEN]{};
  uint16_t trail_x_[TRAIL_LEN]{};
  uint16_t trail_y_[TRAIL_LEN]{};
  uint8_t trail_len_{0};
  bool text_pixels_[Matrix::NUM_LEDS]{};
  bool target_[Matrix::NUM_LEDS]{};
  uint32_t last_update_{0};
  uint8_t last_hour_{255};
  uint8_t last_minute_{255};
};

}  // namespace oraquadra
}  // namespace esphome
