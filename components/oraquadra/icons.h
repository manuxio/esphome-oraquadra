#pragma once

// =============================================================================
// 16×16 monochrome icon library.
//
// Each icon is a `uint16_t bitmap[16]` — one row per array element, MSB on
// the left. All bitmaps live in .rodata.
//
// Look-up by name string (the JSON `icon` field). Names not found return
// nullptr and the renderer falls back to scrolling text only.
//
// To add an icon: drop a new entry in icons.cpp's ICONS table. The
// tools/icon_designer.py script converts a 16×16 PNG into the C bitmap.
// =============================================================================

#include <cstdint>
#include <cstddef>

namespace esphome {
namespace oraquadra {

struct Icon {
  const char *name;          // JSON key, e.g. "warning", "doorbell"
  const uint16_t bitmap[16]; // each row: bit 15 = leftmost pixel
};

extern const Icon ICONS[];
extern const size_t ICONS_COUNT;

// Returns a pointer to the matching icon, or nullptr if no match.
const Icon *find_icon(const char *name);

}  // namespace oraquadra
}  // namespace esphome
