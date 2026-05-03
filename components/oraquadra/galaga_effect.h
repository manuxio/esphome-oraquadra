#pragma once

// Galaga: rainbow shots are launched from the bottom row, fly upward toward
// the time-string letters and "paint" them in a random text colour on impact.
// Once every letter has been hit, the effect holds the lit time for ~5 s
// (with occasional decorative shots) before going dormant until next minute.

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include "esp_random.h"
#include "effect.h"
#include "italian.h"

namespace esphome {
namespace oraquadra {

class GalagaEffect final : public Effect {
 public:
  const char *name() const override { return "Galaga"; }

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

    if (!effect_active_) {
      // Show captured time pixels, no shots.
      for (uint16_t i = 0; i < Matrix::NUM_LEDS; i++) {
        if (text_pixels_[i]) m.set_pixel(i, text_color_);
      }
      return;
    }

    // Post-completion hold window.
    if (completion_time_ != 0 && now_ms - completion_time_ < WAIT_AFTER_MS) {
      for (uint16_t i = 0; i < Matrix::NUM_LEDS; i++) {
        if (text_pixels_[i]) m.set_pixel(i, text_color_);
      }
      // Decorative shots even during hold.
      if (esp_random() % 100 < 10) create_new_shot_();
      // Still draw active shots (decorative).
      draw_shots_(m);
      return;
    }
    if (completion_time_ != 0 && now_ms - completion_time_ >= WAIT_AFTER_MS) {
      effect_active_ = false;
      return;
    }

    if (now_ms - last_update_ < SPEED_MS) {
      // Render last frame state.
      for (uint16_t i = 0; i < Matrix::NUM_LEDS; i++) {
        if (text_pixels_[i]) m.set_pixel(i, text_color_);
      }
      draw_shots_(m);
      return;
    }
    last_update_ = now_ms;

    if (esp_random() % 100 < 15) create_new_shot_();

    int active_shots = 0;
    for (uint8_t i = 0; i < MAX_SHOTS; i++) {
      Shot &sh = shots_[i];
      if (!sh.active) continue;
      active_shots++;

      if (now_ms - sh.last_move > (100u / sh.speed)) {
        sh.x += sh.dx;
        sh.y += sh.dy;
        sh.last_move = now_ms;

        if (sh.x < 0) {
          sh.x = 0;
          sh.dx = 1;
        } else if (sh.x >= static_cast<int8_t>(Matrix::WIDTH)) {
          sh.x = Matrix::WIDTH - 1;
          sh.dx = -1;
        }
        if (sh.y < 0) {
          sh.active = false;
          continue;
        }

        if (std::abs(sh.x - sh.target_x) <= 1 &&
            std::abs(sh.y - sh.target_y) <= 1) {
          find_target_for_shot_(sh);
        }
      }

      // Draw shot as a vertical line of pixels with rainbow hue.
      for (uint8_t j = 0; j < SHOT_LENGTH; j++) {
        int drawX = sh.x;
        int drawY = sh.y - j;
        if (drawY >= 0 && drawY < static_cast<int>(Matrix::HEIGHT)) {
          uint16_t pos = Matrix::xy_to_led(static_cast<uint8_t>(drawX),
                                           static_cast<uint8_t>(drawY));
          if (pos < Matrix::NUM_LEDS) {
            uint8_t shot_h = (shot_hue_ + (i * 30)) & 0xFF;
            m.set_pixel(pos, hsv_(shot_h, 255, 255));
            if (target_[pos] && !text_pixels_[pos]) {
              text_pixels_[pos] = true;
            }
          }
        }
      }
    }
    shot_hue_ += 2;

    for (uint16_t i = 0; i < Matrix::NUM_LEDS; i++) {
      if (text_pixels_[i]) m.set_pixel(i, text_color_);
    }

    bool all_colored = true;
    for (uint16_t i = 0; i < Matrix::NUM_LEDS; i++) {
      if (target_[i] && !text_pixels_[i]) {
        all_colored = false;
        break;
      }
    }

    if (all_colored && active_shots == 0 && completion_time_ == 0) {
      completion_time_ = now_ms;
    }
    if (!all_colored && active_shots < 3 && esp_random() % 100 < 30) {
      create_new_shot_();
    }
  }

 private:
  static constexpr uint8_t MAX_SHOTS = 8;
  static constexpr uint8_t SHOT_LENGTH = 3;
  static constexpr uint32_t SPEED_MS = 30;
  static constexpr uint32_t WAIT_AFTER_MS = 5000;

  struct Shot {
    int8_t x{0};
    int8_t y{0};
    int8_t target_x{0};
    int8_t target_y{0};
    int8_t dx{0};
    int8_t dy{0};
    bool active{false};
    uint8_t speed{1};
    uint32_t last_move{0};
  };

  void init_(const ClockState &s) {
    effect_active_ = true;
    completion_time_ = 0;
    text_hue_ = static_cast<uint8_t>(esp_random() & 0xFF);
    text_color_ = hsv_(text_hue_, 255, 255);
    std::memset(text_pixels_, 0, sizeof(text_pixels_));

    for (uint8_t i = 0; i < MAX_SHOTS; i++) {
      shots_[i].active = false;
      shots_[i].last_move = 0;
    }

    std::memset(target_, 0, sizeof(target_));
    mark_word_(it_words::PREFIX);
    mark_word_(it_words::HOURS[s.hour]);
    if (s.minute > 0) {
      mark_word_(it_words::AND_WORD);
      mark_minutes_(s.minute);
      mark_word_(it_words::SUFFIX);
    }

    for (uint8_t i = 0; i < 3; i++) create_new_shot_();
  }

  void create_new_shot_() {
    for (uint8_t i = 0; i < MAX_SHOTS; i++) {
      if (!shots_[i].active) {
        shots_[i].x = static_cast<int8_t>(esp_random() % Matrix::WIDTH);
        shots_[i].y = Matrix::HEIGHT - 1;
        find_target_for_shot_(shots_[i]);
        shots_[i].active = true;
        shots_[i].speed = static_cast<uint8_t>(2 + (esp_random() % 3));
        shots_[i].last_move = 0;
        return;
      }
    }
  }

  void find_target_for_shot_(Shot &sh) {
    for (int y = 0; y < static_cast<int>(Matrix::HEIGHT); y++) {
      for (int x = 0; x < static_cast<int>(Matrix::WIDTH); x++) {
        // The original walks raw row*WIDTH+x to test target_pixels[]; we
        // compute via xy_to_led so we hit the same physical LEDs.
        uint16_t pos = Matrix::xy_to_led(static_cast<uint8_t>(x), static_cast<uint8_t>(y));
        if (target_[pos] && !text_pixels_[pos]) {
          if (esp_random() % 100 < 30) {
            sh.target_x = static_cast<int8_t>(x);
            sh.target_y = static_cast<int8_t>(y);
            calc_dir_(sh);
            return;
          }
        }
      }
    }
    sh.target_x = sh.x;
    sh.target_y = 0;
    sh.dx = 0;
    sh.dy = -1;
  }

  void calc_dir_(Shot &sh) {
    int diffX = sh.target_x - sh.x;
    int diffY = sh.target_y - sh.y;
    sh.dx = (diffX > 0) ? 1 : (diffX < 0 ? -1 : 0);
    sh.dy = (diffY > 0) ? 1 : (diffY < 0 ? -1 : 0);
    if (diffY < 0) {
      sh.dy = -1;
      if (std::abs(diffX) > 2) {
        sh.dx = (diffX > 0) ? 1 : -1;
      } else {
        sh.dx = 0;
      }
    }
  }

  void draw_shots_(Matrix &m) {
    for (uint8_t i = 0; i < MAX_SHOTS; i++) {
      Shot &sh = shots_[i];
      if (!sh.active) continue;
      for (uint8_t j = 0; j < SHOT_LENGTH; j++) {
        int drawY = sh.y - j;
        if (drawY >= 0 && drawY < static_cast<int>(Matrix::HEIGHT)) {
          uint16_t pos = Matrix::xy_to_led(static_cast<uint8_t>(sh.x),
                                           static_cast<uint8_t>(drawY));
          if (pos < Matrix::NUM_LEDS) {
            uint8_t shot_h = (shot_hue_ + (i * 30)) & 0xFF;
            m.set_pixel(pos, hsv_(shot_h, 255, 255));
          }
        }
      }
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
  bool effect_active_{true};
  uint32_t completion_time_{0};
  uint32_t last_update_{0};
  uint8_t shot_hue_{0};
  uint8_t text_hue_{0};
  Color text_color_{255, 255, 255};
  Shot shots_[MAX_SHOTS]{};
  bool text_pixels_[Matrix::NUM_LEDS]{};
  bool target_[Matrix::NUM_LEDS]{};
  uint8_t last_minute_{255};
};

}  // namespace oraquadra
}  // namespace esphome
