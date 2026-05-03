#pragma once

// =============================================================================
// Words effect — the "default" word-clock display. Delegates time grammar to
// the active Language and overlays an optional per-second heartbeat on the
// LED designated by Language::heartbeat_led().
// =============================================================================

#include "effect.h"

namespace esphome {
namespace oraquadra {

class WordsEffect final : public Effect {
 public:
  const char *name() const override { return "Words"; }

  void update(uint32_t /*now_ms*/, Matrix &m, const ClockState &s) override {
    if (s.language == nullptr) return;
    s.language->render_time(m, s.hour, s.minute, s.color);

    if (s.blink_seconds) {
      const int16_t hb = s.language->heartbeat_led();
      // Even seconds → off; odd seconds → lit. The "lit" state is already set
      // by render_time (the heartbeat LED is part of the time rendering), so
      // we only need to clear it on even seconds.
      if (hb >= 0 && (s.second % 2) == 0) {
        m.set_pixel(static_cast<uint16_t>(hb), Color::BLACK);
      }
    }
  }
};

}  // namespace oraquadra
}  // namespace esphome
