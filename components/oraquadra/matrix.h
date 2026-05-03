#pragma once

// =============================================================================
// 16×16 LED matrix abstraction for OraQuadra.
//
// Wraps an ESPHome `AddressableLight` with:
//   - serpentine xy↔led conversion (the matrix is wired in boustrophedon order)
//   - word painting (multiple LEDs at once)
//   - cornice (60-LED perimeter ring) helpers for the analog clock and the
//     IAQ status frame
//   - inner 14×14 region helpers (everything except the cornice)
//
// The Matrix does NOT own the LED strip; it borrows the pointer set by the
// component on setup(). All writes go straight into the strip's buffer; the
// caller is responsible for committing via show().
// =============================================================================

#include <cstdint>
#include "esphome/core/color.h"
#include "esphome/components/light/addressable_light.h"
#include "language.h"  // for Word

namespace esphome {
namespace oraquadra {

class Matrix {
 public:
  static constexpr uint8_t WIDTH       = 16;
  static constexpr uint8_t HEIGHT      = 16;
  static constexpr uint16_t NUM_LEDS   = WIDTH * HEIGHT;
  static constexpr uint8_t CORNICE_LEN = 60;  // 16 top + 15 right + 15 bottom + 14 left

  explicit Matrix(light::AddressableLight *strip) : strip_(strip) {}

  // ---- bulk operations -----------------------------------------------------
  void clear();              // zero out every pixel
  void clear_inner();        // zero out the inner 14×14 (cornice preserved)
  void fill(Color c);        // every pixel = c
  void apply_global_brightness(uint8_t b);  // scale every pixel by b/255
  void show();               // commit buffer to the LED strip

  // ---- pixel access --------------------------------------------------------
  void set_pixel(uint16_t led, Color c);
  void set_pixel(uint8_t x, uint8_t y, Color c) { set_pixel(xy_to_led(x, y), c); }

  // ---- word painting -------------------------------------------------------
  void paint_word(const Word &w, Color c);

  // ---- cornice (analog hands, IAQ frame, seconds tick) ---------------------
  // `cornice_index` is in [0, 60). The mapping to a physical LED index is
  // fixed by the static CORNICE table below.
  void set_cornice(uint8_t cornice_index, Color c);
  void paint_cornice_uniform(Color c);
  uint16_t cornice_led(uint8_t cornice_index) const;

  // Maps clock minute (0..59) to a cornice index, accounting for which
  // cornice slot is "12 o'clock" and which way the ring turns.
  // Returns a value in [0, 60).
  static uint8_t minute_to_cornice(uint8_t minute);

  // ---- coordinate conversion ----------------------------------------------
  // Serpentine: even rows go right→left, odd rows go left→right.
  static constexpr uint16_t xy_to_led(uint8_t x, uint8_t y) {
    return (y % 2 == 0) ? (y * WIDTH + (WIDTH - 1 - x))
                        : (y * WIDTH + x);
  }

 private:
  light::AddressableLight *strip_;

  // Static perimeter mapping: cornice_index → LED index. Generated to match
  // the V1.2.9 hardware orientation (start at top-left, go clockwise).
  static const uint16_t CORNICE[CORNICE_LEN];
};

// "12 o'clock" position on the cornice ring. Adjust if the matrix is mounted
// rotated. Could be made YAML-configurable later.
inline constexpr uint8_t CORNICE_TOP_OFFSET = 8;
// CORNICE array traversal happens to go counter-clockwise on this serpentine
// matrix layout (top R→L, left T→B, bottom L→R, right B→T). For the analog
// clock we want hands to move clockwise, so we INVERT the minute index.
inline constexpr bool    CORNICE_CLOCKWISE  = false;

}  // namespace oraquadra
}  // namespace esphome
