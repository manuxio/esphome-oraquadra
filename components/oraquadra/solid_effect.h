#pragma once

// =============================================================================
// Solid effect — fills the entire matrix with the user-selected color.
//
// Selectable from HA via select.<node>_effect = "Solid". Useful as a "turn the
// whole matrix on" action when used together with a non-zero brightness.
// =============================================================================

#include "effect.h"

namespace esphome {
namespace oraquadra {

class SolidEffect final : public Effect {
 public:
  const char *name() const override { return "Solid"; }

  void update(uint32_t /*now_ms*/, Matrix &m, const ClockState &s) override {
    m.fill(s.color);
  }
};

}  // namespace oraquadra
}  // namespace esphome
