#pragma once

// Tron: 3 light-bikes (cyan / orange / violet) wander the matrix avoiding
// each other's trails. After ~8 s the bikes vanish and the time is shown
// solid until the minute changes.

#include <cstdint>
#include <cstring>
#include "esp_random.h"
#include "effect.h"
#include "italian.h"

namespace esphome {
namespace oraquadra {

class TronEffect final : public Effect {
 public:
  const char *name() const override { return "Tron"; }

  void on_minute_change(uint8_t /*h*/, uint8_t /*m*/) override {
    needs_init_ = true;
    effect_active_ = true;
    cycle_start_ = 0;
  }

  void update(uint32_t now_ms, Matrix &m, const ClockState &s) override {
    if (s.language == nullptr) return;

    // Once the active phase has elapsed, just show the time.
    if (!effect_active_ && !needs_init_) {
      paint_time_(m, s);
      return;
    }

    if (cycle_start_ == 0) cycle_start_ = now_ms;
    uint32_t cycle_elapsed = now_ms - cycle_start_;

    if (effect_active_ && cycle_elapsed > EFFECT_DURATION_MS) {
      effect_active_ = false;
      cycle_start_ = now_ms;
      needs_init_ = false;
      paint_time_(m, s);
      return;
    }

    if (needs_init_) {
      init_bikes_();
    }

    if (now_ms - last_update_ < UPDATE_INTERVAL_MS) {
      // Still re-paint so the picture stays on the buffer between ticks.
      paint_frame_(m, s);
      return;
    }
    last_update_ = now_ms;

    // Fade existing trails.
    for (uint16_t i = 0; i < Matrix::NUM_LEDS; i++) {
      intensity_[i] = (intensity_[i] > 15) ? intensity_[i] - 15 : 0;
    }

    // Move each bike.
    for (uint8_t i = 0; i < NUM_BIKES; i++) {
      Bike &bike = bikes_[i];
      if (!bike.active) continue;

      int8_t newDir = find_safe_dir_(i);
      if (newDir < 0) {
        // Bike trapped — teleport.
        bike.x = static_cast<int8_t>(esp_random() % Matrix::WIDTH);
        bike.y = static_cast<int8_t>(esp_random() % Matrix::HEIGHT);
        bike.dir = static_cast<uint8_t>(esp_random() % 4);
        bike.trail_len = 0;
        continue;
      }
      bike.dir = static_cast<uint8_t>(newDir);

      static const int8_t dx_tab[4] = {0, 1, 0, -1};
      static const int8_t dy_tab[4] = {-1, 0, 1, 0};

      // Save current head position into the trail buffer.
      if (bike.trail_len < TRAIL_LEN) {
        bike.trailX[bike.trail_len] = static_cast<uint8_t>(bike.x);
        bike.trailY[bike.trail_len] = static_cast<uint8_t>(bike.y);
        bike.trail_len++;
      } else {
        for (uint8_t t = 0; t < TRAIL_LEN - 1; t++) {
          bike.trailX[t] = bike.trailX[t + 1];
          bike.trailY[t] = bike.trailY[t + 1];
        }
        bike.trailX[TRAIL_LEN - 1] = static_cast<uint8_t>(bike.x);
        bike.trailY[TRAIL_LEN - 1] = static_cast<uint8_t>(bike.y);
      }

      uint16_t pos = Matrix::xy_to_led(static_cast<uint8_t>(bike.x), static_cast<uint8_t>(bike.y));
      if (pos < Matrix::NUM_LEDS) intensity_[pos] = 255;

      bike.x += dx_tab[bike.dir];
      bike.y += dy_tab[bike.dir];

      if (bike.x < 0) bike.x = Matrix::WIDTH - 1;
      if (bike.x >= static_cast<int8_t>(Matrix::WIDTH)) bike.x = 0;
      if (bike.y < 0) bike.y = Matrix::HEIGHT - 1;
      if (bike.y >= static_cast<int8_t>(Matrix::HEIGHT)) bike.y = 0;
    }

    paint_frame_(m, s);
  }

 private:
  static constexpr uint8_t NUM_BIKES = 3;
  static constexpr uint8_t TRAIL_LEN = 12;
  static constexpr uint32_t UPDATE_INTERVAL_MS = 80;
  static constexpr uint32_t EFFECT_DURATION_MS = 8000;

  struct Bike {
    int8_t x{0};
    int8_t y{0};
    uint8_t dir{0};      // 0=up, 1=right, 2=down, 3=left
    Color color{0, 0, 0};
    bool active{false};
    uint8_t trailX[TRAIL_LEN]{};
    uint8_t trailY[TRAIL_LEN]{};
    uint8_t trail_len{0};
  };

  void init_bikes_() {
    Color colors[NUM_BIKES] = {
        Color{0, 200, 255},
        Color{255, 100, 0},
        Color{200, 0, 255},
    };
    for (uint8_t i = 0; i < NUM_BIKES; i++) {
      bikes_[i].active = true;
      bikes_[i].color = colors[i];
      bikes_[i].trail_len = 0;
      switch (i) {
        case 0:
          bikes_[i].x = 0;
          bikes_[i].y = 4;
          bikes_[i].dir = 1;
          break;
        case 1:
          bikes_[i].x = Matrix::WIDTH - 1;
          bikes_[i].y = 11;
          bikes_[i].dir = 3;
          break;
        case 2:
          bikes_[i].x = 8;
          bikes_[i].y = Matrix::HEIGHT - 1;
          bikes_[i].dir = 0;
          break;
      }
    }
    std::memset(intensity_, 0, sizeof(intensity_));
    needs_init_ = false;
  }

  bool pos_is_free_(int8_t x, int8_t y) const {
    if (x < 0 || x >= static_cast<int8_t>(Matrix::WIDTH)) return false;
    if (y < 0 || y >= static_cast<int8_t>(Matrix::HEIGHT)) return false;
    uint16_t pos = Matrix::xy_to_led(static_cast<uint8_t>(x), static_cast<uint8_t>(y));
    if (pos >= Matrix::NUM_LEDS) return false;
    if (intensity_[pos] > 50) return false;
    return true;
  }

  int8_t find_safe_dir_(uint8_t bike_idx) {
    Bike &bike = bikes_[bike_idx];
    static const int8_t dx_tab[4] = {0, 1, 0, -1};
    static const int8_t dy_tab[4] = {-1, 0, 1, 0};

    int8_t newX = bike.x + dx_tab[bike.dir];
    int8_t newY = bike.y + dy_tab[bike.dir];
    if (pos_is_free_(newX, newY)) {
      if (esp_random() % 100 < 70) {
        return static_cast<int8_t>(bike.dir);
      }
    }

    int8_t possible[3];
    uint8_t num_possible = 0;
    for (int8_t d = 0; d < 4; d++) {
      if ((d + 2) % 4 == static_cast<int8_t>(bike.dir)) continue;  // no U-turn
      newX = bike.x + dx_tab[d];
      newY = bike.y + dy_tab[d];
      if (pos_is_free_(newX, newY)) {
        possible[num_possible++] = d;
      }
    }

    if (num_possible > 0) {
      return possible[esp_random() % num_possible];
    }
    return -1;
  }

  void paint_frame_(Matrix &m, const ClockState &s) {
    // Trails (default cyan-ish per original, CHSV(160) ~ blue/cyan).
    for (uint16_t i = 0; i < Matrix::NUM_LEDS; i++) {
      if (intensity_[i] > 0) {
        // CHSV(160, 255, intensity) — sector 3 (blue→cyan transition):
        // hue 160/255 => 226 deg => blue with hint of green.
        // Approximate with a fixed cyan tinted by intensity.
        m.set_pixel(i, Color{
            static_cast<uint8_t>((0 * intensity_[i]) >> 8),
            static_cast<uint8_t>((128 * intensity_[i]) >> 8),
            static_cast<uint8_t>((255 * intensity_[i]) >> 8),
        });
      }
    }

    // Heads + colored trail per-bike.
    for (uint8_t i = 0; i < NUM_BIKES; i++) {
      Bike &bike = bikes_[i];
      if (!bike.active) continue;

      uint16_t pos = Matrix::xy_to_led(static_cast<uint8_t>(bike.x), static_cast<uint8_t>(bike.y));
      if (pos < Matrix::NUM_LEDS) {
        // maximizeBrightness scales to make max channel = 255.
        m.set_pixel(pos, max_brightness_(bike.color));
      }

      for (uint8_t t = 0; t < bike.trail_len; t++) {
        uint16_t tp = Matrix::xy_to_led(bike.trailX[t], bike.trailY[t]);
        if (tp < Matrix::NUM_LEDS) {
          uint8_t s_ = (180 > t * 12) ? static_cast<uint8_t>(180 - t * 12) : 0;
          m.set_pixel(tp, Color{
              static_cast<uint8_t>((bike.color.r * s_) >> 8),
              static_cast<uint8_t>((bike.color.g * s_) >> 8),
              static_cast<uint8_t>((bike.color.b * s_) >> 8),
          });
        }
      }
    }

    paint_time_(m, s);
  }

  static Color max_brightness_(Color c) {
    uint8_t mx = c.r;
    if (c.g > mx) mx = c.g;
    if (c.b > mx) mx = c.b;
    if (mx == 0 || mx == 255) return c;
    uint32_t scale = (255u * 256u) / mx;  // q8.8 fixed point
    auto clamp = [](uint32_t v) -> uint8_t { return v > 255 ? 255 : static_cast<uint8_t>(v); };
    return Color{
        clamp((static_cast<uint32_t>(c.r) * scale) >> 8),
        clamp((static_cast<uint32_t>(c.g) * scale) >> 8),
        clamp((static_cast<uint32_t>(c.b) * scale) >> 8),
    };
  }

  void paint_time_(Matrix &m, const ClockState &s) {
    // Original showCurrentTime() draws everything in WHITE on Tron, regardless
    // of user color. Replicate that for visual fidelity.
    Color white{255, 255, 255};
    m.paint_word(it_words::PREFIX, white);
    m.paint_word(it_words::HOURS[s.hour], white);
    if (s.minute > 0) {
      m.paint_word(it_words::AND_WORD, white);
      s.language->render_phase(m, s.hour, s.minute, white, 3);
      m.paint_word(it_words::SUFFIX, white);
    }
  }

  Bike bikes_[NUM_BIKES]{};
  uint8_t intensity_[Matrix::NUM_LEDS]{};
  bool effect_active_{true};
  bool needs_init_{true};
  uint32_t cycle_start_{0};
  uint32_t last_update_{0};
};

}  // namespace oraquadra
}  // namespace esphome
