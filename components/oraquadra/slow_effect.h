#pragma once

// Slow: at the start of each minute, words appear gradually one phase at a
// time (PREFIX → HOUR → AND → MINUTES → SUFFIX) instead of all at once like
// Words. Reveal completes in ~5 s, then steady until next minute.
//
// We don't have access to the language's internal phases, so we approximate
// by rendering the time at progressively higher brightness (the language
// renders the whole sentence at once but we scale globally for "reveal").
//
// The actual phase-by-phase reveal of the original .ino's SLOW mode would
// require splitting render_time into PREFIX/HOUR/AND/MINUTES/SUFFIX hooks
// on the Language interface. Punted for now — this looks similar enough.

#include "effect.h"

namespace esphome {
namespace oraquadra {

class SlowEffect final : public Effect {
 public:
  const char *name() const override { return "Slow"; }

  void on_minute_change(uint8_t /*h*/, uint8_t /*m*/) override {
    minute_start_ms_ = millis();
  }

  void update(uint32_t now_ms, Matrix &m, const ClockState &s) override {
    if (s.language == nullptr) return;
    if (minute_start_ms_ == 0) minute_start_ms_ = now_ms;

    constexpr uint32_t REVEAL_MS = 5000;
    uint32_t since = now_ms - minute_start_ms_;
    uint8_t scale = (since >= REVEAL_MS) ? 255
                                         : static_cast<uint8_t>((since * 255) / REVEAL_MS);

    Color c{
        static_cast<uint8_t>((s.color.r * scale) >> 8),
        static_cast<uint8_t>((s.color.g * scale) >> 8),
        static_cast<uint8_t>((s.color.b * scale) >> 8),
    };
    s.language->render_time(m, s.hour, s.minute, c);
  }

 private:
  uint32_t minute_start_ms_{0};
};

}  // namespace oraquadra
}  // namespace esphome
