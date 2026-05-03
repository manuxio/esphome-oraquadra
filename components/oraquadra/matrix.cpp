#include "matrix.h"

namespace esphome {
namespace oraquadra {

// Cornice = 60 LED indices traced clockwise around the perimeter, starting at
// the top-left corner. Verified against the V1.2.9 layout:
//   16 top + 15 right + 15 bottom + 14 left = 60.
const uint16_t Matrix::CORNICE[Matrix::CORNICE_LEN] = {
    // top edge
    0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
    // right edge (15 LEDs, top → bottom)
    16,  47,  48,  79,  80, 111, 112, 143, 144, 175, 176, 207, 208, 239, 240,
    // bottom edge (15 LEDs, left → right)
    241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255,
    // left edge (14 LEDs, bottom → top)
    224, 223, 192, 191, 160, 159, 128, 127,  96,  95,  64,  63,  32,  31,
};

void Matrix::clear() {
  for (uint16_t i = 0; i < NUM_LEDS; i++)
    (*strip_)[i] = Color::BLACK;
}

void Matrix::clear_inner() {
  // The inner 14×14 starts at (1, 1) and ends at (14, 14).
  for (uint8_t y = 1; y < HEIGHT - 1; y++) {
    for (uint8_t x = 1; x < WIDTH - 1; x++) {
      (*strip_)[xy_to_led(x, y)] = Color::BLACK;
    }
  }
}

void Matrix::show() {
  strip_->schedule_show();
}

void Matrix::set_pixel(uint16_t led, Color c) {
  if (led >= NUM_LEDS) return;
  (*strip_)[led] = c;
}

void Matrix::paint_word(const Word &w, Color c) {
  for (uint8_t i = 0; i < w.count; i++) {
    set_pixel(w.leds[i], c);
  }
}

void Matrix::set_cornice(uint8_t cornice_index, Color c) {
  if (cornice_index >= CORNICE_LEN) return;
  set_pixel(CORNICE[cornice_index], c);
}

void Matrix::paint_cornice_uniform(Color c) {
  for (uint8_t i = 0; i < CORNICE_LEN; i++)
    set_pixel(CORNICE[i], c);
}

uint16_t Matrix::cornice_led(uint8_t cornice_index) const {
  return (cornice_index < CORNICE_LEN) ? CORNICE[cornice_index] : 0;
}

uint8_t Matrix::minute_to_cornice(uint8_t minute) {
  const uint8_t base = CORNICE_CLOCKWISE ? minute : (60 - minute) % 60;
  return (base + CORNICE_TOP_OFFSET) % CORNICE_LEN;
}

}  // namespace oraquadra
}  // namespace esphome
