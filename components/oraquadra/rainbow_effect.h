#pragma once

// Rainbow: paints each grammatical chunk in a different hue stepped 40 hue
// units apart, with the global hue scrolling once every ~50 ms.

#include <cstdint>
#include "effect.h"
#include "italian.h"

namespace esphome {
namespace oraquadra {

class RainbowEffect final : public Effect {
 public:
  const char *name() const override { return "Rainbow"; }

  void update(uint32_t now_ms, Matrix &m, const ClockState &s) override {
    if (s.language == nullptr) return;

    if (now_ms - last_update_ >= UPDATE_INTERVAL_MS) {
      last_update_ = now_ms;
      hue_++;
    }

    uint8_t off = 0;
    m.paint_word(it_words::PREFIX, hsv_(hue_ + off, 255, 255));
    off += 40;
    m.paint_word(it_words::HOURS[s.hour], hsv_(hue_ + off, 255, 255));
    off += 40;

    if (s.minute > 0) {
      m.paint_word(it_words::AND_WORD, hsv_(hue_ + off, 255, 255));
      off += 40;
      s.language->render_phase(m, s.hour, s.minute, hsv_(hue_ + off, 255, 255), 3);
      off += 40;
      m.paint_word(it_words::SUFFIX, hsv_(hue_ + off, 255, 255));
      off += 40;

      // Universal seconds blink is painted by render_() — no per-effect heartbeat.
    }
  }

 private:
  static constexpr uint32_t UPDATE_INTERVAL_MS = 50;

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

  uint32_t last_update_{0};
  uint8_t hue_{0};
};

}  // namespace oraquadra
}  // namespace esphome
