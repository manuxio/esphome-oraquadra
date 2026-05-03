#pragma once

// =============================================================================
// Effect interface — every visualisation mode (words, analog, matrix-rain,
// pacman, etc.) implements this. The component owns a registry of Effects and
// dispatches the active one each frame.
// =============================================================================

#include <cstdint>
#include "esphome/core/color.h"
#include "matrix.h"
#include "language.h"

namespace esphome {
namespace oraquadra {

// Read-only snapshot passed to every effect on each tick.
struct ClockState {
  uint8_t hour{0};
  uint8_t minute{0};
  uint8_t second{0};
  Color   color{255, 255, 255};   // user-selected foreground
  bool    blink_seconds{true};
  uint16_t scroll_speed_ms{50};
  const Language *language{nullptr};
};

class Effect {
 public:
  virtual ~Effect() = default;

  // Stable name used in logs and as the HA select option label.
  virtual const char *name() const = 0;

  // Called by the component when the wall-clock minute rolls over. Effects
  // that animate per-minute reset their state here.
  virtual void on_minute_change(uint8_t hour, uint8_t minute) {}

  // Renders one frame. The matrix has been cleared by the component (or has
  // an IAQ base layer painted) — the effect just paints on top.
  virtual void update(uint32_t now_ms, Matrix &matrix, const ClockState &state) = 0;

  // Returns true when the effect has finished a "natural" cycle and is ready
  // to yield to the digital overlay (or to be replaced). Continuous effects
  // (rainbow, matrix-rain) should always return false.
  virtual bool cycle_completed() const { return false; }
};

}  // namespace oraquadra
}  // namespace esphome
