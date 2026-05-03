#pragma once

// Scroll effect — renders the user-configured scroll_text as a horizontal
// scrolling banner on the matrix. Centered vertically (5x7 font in rows 4-10).
// Loops indefinitely.

#include "effect.h"
#include "fonts.h"
#include <cstring>
#include <string>

namespace esphome {
namespace oraquadra {

class ScrollEffect final : public Effect {
 public:
  const char *name() const override { return "Scroll"; }

  // Owns its own copy of the text so the pointer stays valid even if the
  // caller's std::string is destroyed (the caller usually passes a temporary
  // const-ref parameter).
  void set_text(const char *text) {
    text_ = (text != nullptr) ? std::string(text) : std::string();
    start_ms_ = millis();
  }

  void update(uint32_t now_ms, Matrix &m, const ClockState &s) override {
    if (text_.empty()) {
      // Nothing to show — render hint text so the user sees the mode is
      // selected but waiting for input.
      static const char *hint = "SET TEXT IN HA";
      render_(m, hint, std::strlen(hint), now_ms, s.color, s.scroll_speed_ms);
      return;
    }
    render_(m, text_.c_str(), text_.size(), now_ms, s.color, s.scroll_speed_ms);
  }

 private:
  std::string text_;
  uint32_t start_ms_{0};

  void render_(Matrix &m, const char *t, size_t n, uint32_t now_ms,
               Color c, uint16_t speed_ms) {
    // Each char = 5 cols + 1 col gap = 6 cols. Total scroll width is
    // 6*n + 16 (16 cols of leading + trailing padding for clean enter/exit).
    const int16_t total_cols = static_cast<int16_t>(6 * n) + 16;
    const uint16_t step_ms = (speed_ms > 0) ? speed_ms : 80;
    const uint32_t elapsed = now_ms - start_ms_;
    int16_t offset = static_cast<int16_t>(elapsed / step_ms);
    // Loop: when we've scrolled past the full text, reset.
    if (offset >= total_cols) {
      start_ms_ = now_ms;
      offset = 0;
    }
    const int16_t pos = 16 - offset;  // x of first char's first column

    constexpr uint8_t y0 = 4;  // vertically centered (rows 4-10 of 16)
    for (size_t i = 0; i < n; i++) {
      const uint8_t *glyph = font5x7_glyph(t[i]);
      const int16_t glyph_x = pos + static_cast<int16_t>(i * 6);
      for (uint8_t col = 0; col < FONT5x7_WIDTH; col++) {
        const int16_t x = glyph_x + col;
        if (x < 0 || x >= (int16_t) Matrix::WIDTH) continue;
        const uint8_t bits = glyph[col];
        for (uint8_t row = 0; row < FONT5x7_HEIGHT; row++) {
          if (bits & (1 << row)) {
            m.set_pixel(static_cast<uint8_t>(x), y0 + row, c);
          }
        }
      }
    }
  }
};

}  // namespace oraquadra
}  // namespace esphome
