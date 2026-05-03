#pragma once

// =============================================================================
// Pluggable language interface for the word-clock display.
//
// A Language owns three things:
//   1. The vocabulary — which LEDs spell which word on the physical matrix.
//   2. The grammar    — how the time is composed (which words, in what order).
//   3. Per-language UI hints (heartbeat LED for blink-seconds, etc).
//
// The interface deliberately passes the raw (hour, minute) pair and lets each
// language decide the rest. This accommodates wildly different grammars:
//
//   Italian   3:20  →  "SONO LE TRE E VENTI MINUTI"  (additive: H + AND + M + LABEL)
//   English   3:20  →  "TWENTY PAST THREE"           (split at :30, PAST/TO)
//   English   3:45  →  "QUARTER TO FOUR"             (TO branch bumps the hour)
//   English   0:20  →  "TWENTY PAST MIDNIGHT"        (special hour word)
//   German    3:30  →  "ES IST HALB VIER"            (half FOUR = 3:30, hour++)
//   German    3:45  →  "VIERTEL VOR VIER"            (or "DREIVIERTEL VIER")
//
// Adding a new language = drop a new file under languages/ that implements
// this interface; nothing else in the firmware changes.
//
// The physical 16×16 grid of OraQuadra V1.2.9 only supports Italian (alphabet
// missing B and W; contiguous letters tuned for Italian time words). A
// different faceplate would warrant a new language module here.
// =============================================================================

#include <cstdint>
#include "esphome/core/color.h"

namespace esphome {
namespace oraquadra {

// A vocabulary entry: the list of LED indices that, when lit together, spell
// one word on the matrix.
struct Word {
  const uint8_t *leds;
  uint8_t count;
};

class Matrix;  // forward declaration — defined in matrix.h

class Language {
 public:
  virtual ~Language() = default;

  // Paints the current time onto `matrix` using the language's own grammar.
  // The caller has already cleared the matrix; the language only paints the
  // LEDs it needs lit. Implementations are free to:
  //   - skip "minute" rendering entirely on the hour (mm == 0)
  //   - switch grammar branches (e.g. PAST/TO at minute 30)
  //   - bump the hour ("half past three" → some langs render hour+1)
  //   - use language-specific words for 0/12 (MIDNIGHT, NOON, MEZZOGIORNO…)
  virtual void render_time(Matrix &matrix, uint8_t hour, uint8_t minute,
                           Color color) const = 0;

  // ISO 639-1 code, e.g. "it", "en", "de". Used for diagnostics and the HA
  // attribute on the language entity.
  virtual const char *code() const = 0;

  // Human-readable name in English: "Italian", "English", "German".
  virtual const char *name() const = 0;

  // LED index to pulse once per second as a heartbeat / blink-seconds
  // indicator. Typically a small letter that is already part of the time
  // rendering (Italian uses "E" at LED 116). Return -1 to disable the
  // heartbeat for languages that don't have a suitable LED.
  virtual int16_t heartbeat_led() const { return -1; }

  // Phased rendering — used by FadeEffect and SlowEffect to reveal one
  // grammatical chunk at a time. Phase order for Italian:
  //   0 = PREFIX  ("SONO LE")
  //   1 = HOUR    (numeric hour word)
  //   2 = AND     ("E", connective; only if minute > 0)
  //   3 = MINUTES (tens + units; only if minute > 0)
  //   4 = SUFFIX  ("MINUTI"; only if minute > 0)
  // Default impl renders nothing — base classes that don't support phased
  // rendering can leave it untouched.
  virtual void render_phase(Matrix &matrix, uint8_t hour, uint8_t minute,
                            Color color, uint8_t phase) const {}
  virtual uint8_t total_phases() const { return 5; }
};

}  // namespace oraquadra
}  // namespace esphome
