#pragma once

// =============================================================================
// Italian language module for OraQuadra (V1.2.9 matrix layout).
//
// Identifiers in this file use English cardinals (HOUR_ONE, MIN_TWENTY, ...)
// for code readability. The trailing comment on each LED array shows which
// Italian word that constant actually lights up on the physical matrix.
//
// LED indices are derived from the matrix mapping at the top of the original
// .ino (lines 75-90) and live in .rodata (flash) on ESP32 thanks to
// `inline constexpr` at namespace scope.
// =============================================================================

#include "language.h"
#include "matrix.h"

namespace esphome {
namespace oraquadra {

// Italian-specific vocabulary lives in its own namespace to avoid clashes with
// other future languages that might use the same identifier names (HOURS, etc).
namespace it_words {

// Helper: build a Word from a constexpr C array, deriving count from size.
#define ORAQ_WORD(arr) Word{(arr), static_cast<uint8_t>(sizeof(arr) / sizeof((arr)[0]))}

// -----------------------------------------------------------------------------
// Frame / connectives
// -----------------------------------------------------------------------------
inline constexpr uint8_t PREFIX_LEDS[] = {15, 14, 13, 12, 10, 9, 7, 6, 5};       // "SONO LE"
inline constexpr Word    PREFIX        = ORAQ_WORD(PREFIX_LEDS);

inline constexpr uint8_t SUFFIX_LEDS[] = {250, 251, 252, 253, 254, 255};         // "MINUTI"
inline constexpr Word    SUFFIX        = ORAQ_WORD(SUFFIX_LEDS);

inline constexpr uint8_t AND_LEDS[]    = {116};                                  // "E"
inline constexpr Word    AND_WORD      = ORAQ_WORD(AND_LEDS);
inline constexpr uint8_t HEARTBEAT_LED = 116;  // the "E" doubles as the seconds heartbeat

// -----------------------------------------------------------------------------
// Hours 0–23 — words used after PREFIX ("SONO LE …")
// -----------------------------------------------------------------------------
inline constexpr uint8_t HOUR_ZERO_LEDS[]         = {3, 2, 1, 0};                                       // "ZERO"
inline constexpr uint8_t HOUR_ONE_LEDS[]          = {57, 70, 89};                                       // "UNA"
inline constexpr uint8_t HOUR_TWO_LEDS[]          = {111, 112, 143};                                    // "DUE"
inline constexpr uint8_t HOUR_THREE_LEDS[]        = {21, 22, 23};                                       // "TRE"
inline constexpr uint8_t HOUR_FOUR_LEDS[]         = {56, 57, 58, 59, 60, 61, 62};                       // "QUATTRO"
inline constexpr uint8_t HOUR_FIVE_LEDS[]         = {46, 49, 78, 81, 110, 113};                         // "CINQUE"
inline constexpr uint8_t HOUR_SIX_LEDS[]          = {34, 33, 32};                                       // "SEI"
inline constexpr uint8_t HOUR_SEVEN_LEDS[]        = {91, 92, 93, 94, 95};                               // "SETTE"
inline constexpr uint8_t HOUR_EIGHT_LEDS[]        = {28, 29, 30, 31};                                   // "OTTO"
inline constexpr uint8_t HOUR_NINE_LEDS[]         = {101, 100, 99, 98};                                 // "NOVE"
inline constexpr uint8_t HOUR_TEN_LEDS[]          = {68, 67, 66, 65, 64};                               // "DIECI"
inline constexpr uint8_t HOUR_ELEVEN_LEDS[]       = {50, 51, 52, 53, 54, 55};                           // "UNDICI"
inline constexpr uint8_t HOUR_TWELVE_LEDS[]       = {109, 108, 107, 106, 105, 104};                     // "DODICI"
inline constexpr uint8_t HOUR_THIRTEEN_LEDS[]     = {21, 22, 23, 24, 25, 26, 27};                       // "TREDICI"
inline constexpr uint8_t HOUR_FOURTEEN_LEDS[]     = {45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35};       // "QUATTORDICI"
inline constexpr uint8_t HOUR_FIFTEEN_LEDS[]      = {45, 50, 77, 82, 109, 114, 141, 146};               // "QUINDICI"
inline constexpr uint8_t HOUR_SIXTEEN_LEDS[]      = {83, 84, 85, 86, 87, 88};                           // "SEDICI"
inline constexpr uint8_t HOUR_SEVENTEEN_LEDS[]    = {85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95};       // "DICIASSETTE"
inline constexpr uint8_t HOUR_EIGHTEEN_LEDS[]     = {24, 25, 26, 27, 28, 29, 30, 31};                   // "DICIOTTO"
inline constexpr uint8_t HOUR_NINETEEN_LEDS[]     = {107, 106, 105, 104, 103, 102, 101, 100, 99, 98};   // "DICIANNOVE"
inline constexpr uint8_t HOUR_TWENTY_LEDS[]       = {16, 17, 18, 19, 20};                               // "VENTI"
inline constexpr uint8_t HOUR_TWENTY_ONE_LEDS[]   = {75, 74, 73, 72, 71, 70, 69};                       // "VENTUNO"
inline constexpr uint8_t HOUR_TWENTY_TWO_LEDS[]   = {16, 47, 48, 79, 80, 111, 112, 143};                // "VENTIDUE"
inline constexpr uint8_t HOUR_TWENTY_THREE_LEDS[] = {16, 17, 18, 19, 20, 21, 22, 23};                   // "VENTITRE"

inline constexpr Word HOURS[24] = {
    ORAQ_WORD(HOUR_ZERO_LEDS),          ORAQ_WORD(HOUR_ONE_LEDS),
    ORAQ_WORD(HOUR_TWO_LEDS),           ORAQ_WORD(HOUR_THREE_LEDS),
    ORAQ_WORD(HOUR_FOUR_LEDS),          ORAQ_WORD(HOUR_FIVE_LEDS),
    ORAQ_WORD(HOUR_SIX_LEDS),           ORAQ_WORD(HOUR_SEVEN_LEDS),
    ORAQ_WORD(HOUR_EIGHT_LEDS),         ORAQ_WORD(HOUR_NINE_LEDS),
    ORAQ_WORD(HOUR_TEN_LEDS),           ORAQ_WORD(HOUR_ELEVEN_LEDS),
    ORAQ_WORD(HOUR_TWELVE_LEDS),        ORAQ_WORD(HOUR_THIRTEEN_LEDS),
    ORAQ_WORD(HOUR_FOURTEEN_LEDS),      ORAQ_WORD(HOUR_FIFTEEN_LEDS),
    ORAQ_WORD(HOUR_SIXTEEN_LEDS),       ORAQ_WORD(HOUR_SEVENTEEN_LEDS),
    ORAQ_WORD(HOUR_EIGHTEEN_LEDS),      ORAQ_WORD(HOUR_NINETEEN_LEDS),
    ORAQ_WORD(HOUR_TWENTY_LEDS),        ORAQ_WORD(HOUR_TWENTY_ONE_LEDS),
    ORAQ_WORD(HOUR_TWENTY_TWO_LEDS),    ORAQ_WORD(HOUR_TWENTY_THREE_LEDS),
};

// -----------------------------------------------------------------------------
// Minute units 1–19 — physically distinct LED group from hours, since the
// matrix repeats number words in the lower half for "<hour> AND <minutes>".
// -----------------------------------------------------------------------------
inline constexpr uint8_t MIN_ONE_LEDS[]        = {157, 158, 159};                                              // "UNO"
inline constexpr uint8_t MIN_TWO_LEDS[]        = {246, 247, 248};                                              // "DUE"
inline constexpr uint8_t MIN_THREE_LEDS[]      = {210, 211, 212};                                              // "TRE"
inline constexpr uint8_t MIN_FOUR_LEDS[]       = {207, 206, 205, 204, 203, 202, 201};                          // "QUATTRO"
inline constexpr uint8_t MIN_FIVE_LEDS[]       = {240, 241, 242, 243, 244, 245};                               // "CINQUE"
inline constexpr uint8_t MIN_SIX_LEDS[]        = {226, 225, 224};                                              // "SEI"
inline constexpr uint8_t MIN_SEVEN_LEDS[]      = {219, 220, 221, 222, 223};                                    // "SETTE"
inline constexpr uint8_t MIN_EIGHT_LEDS[]      = {163, 162, 161, 160};                                         // "OTTO"
inline constexpr uint8_t MIN_NINE_LEDS[]       = {231, 230, 229, 228};                                         // "NOVE"
inline constexpr uint8_t MIN_TEN_LEDS[]        = {176, 177, 178, 179, 180};                                    // "DIECI"
inline constexpr uint8_t MIN_ELEVEN_LEDS[]     = {239, 238, 237, 236, 235, 234};                               // "UNDICI"
inline constexpr uint8_t MIN_TWELVE_LEDS[]     = {169, 168, 167, 166, 165, 164};                               // "DODICI"
inline constexpr uint8_t MIN_THIRTEEN_LEDS[]   = {210, 211, 212, 213, 214, 215, 216};                          // "TREDICI"
inline constexpr uint8_t MIN_FOURTEEN_LEDS[]   = {181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191};      // "QUATTORDICI"
inline constexpr uint8_t MIN_FIFTEEN_LEDS[]    = {200, 199, 198, 197, 196, 195, 194, 193};                     // "QUINDICI"
inline constexpr uint8_t MIN_SIXTEEN_LEDS[]    = {175, 174, 173, 172, 171, 170};                               // "SEDICI"
inline constexpr uint8_t MIN_SEVENTEEN_LEDS[]  = {213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223};      // "DICIASSETTE"
inline constexpr uint8_t MIN_EIGHTEEN_LEDS[]   = {167, 166, 165, 164, 163, 162, 161, 160};                     // "DICIOTTO"
inline constexpr uint8_t MIN_NINETEEN_LEDS[]   = {237, 236, 235, 234, 233, 232, 231, 230, 229, 228};            // "DICIANNOVE"

// MINUTE_UNITS[m] for m ∈ [1, 19]. Slot [0] is a sentinel placeholder so
// callers can index directly with the minute value without an off-by-one.
inline constexpr Word MINUTE_UNITS[20] = {
    {nullptr, 0},                       // 0 — never read
    ORAQ_WORD(MIN_ONE_LEDS),            ORAQ_WORD(MIN_TWO_LEDS),
    ORAQ_WORD(MIN_THREE_LEDS),          ORAQ_WORD(MIN_FOUR_LEDS),
    ORAQ_WORD(MIN_FIVE_LEDS),           ORAQ_WORD(MIN_SIX_LEDS),
    ORAQ_WORD(MIN_SEVEN_LEDS),          ORAQ_WORD(MIN_EIGHT_LEDS),
    ORAQ_WORD(MIN_NINE_LEDS),           ORAQ_WORD(MIN_TEN_LEDS),
    ORAQ_WORD(MIN_ELEVEN_LEDS),         ORAQ_WORD(MIN_TWELVE_LEDS),
    ORAQ_WORD(MIN_THIRTEEN_LEDS),       ORAQ_WORD(MIN_FOURTEEN_LEDS),
    ORAQ_WORD(MIN_FIFTEEN_LEDS),        ORAQ_WORD(MIN_SIXTEEN_LEDS),
    ORAQ_WORD(MIN_SEVENTEEN_LEDS),      ORAQ_WORD(MIN_EIGHTEEN_LEDS),
    ORAQ_WORD(MIN_NINETEEN_LEDS),
};

// -----------------------------------------------------------------------------
// Tens 20–50. Each has a full form (VENTI, TRENTA, …) and an elided form
// (VENT, TRENT, …) used when the units digit is 1 or 8 — Italian drops the
// trailing vowel: 21 → VENTUNO, 28 → VENTOTTO, etc.
// -----------------------------------------------------------------------------
inline constexpr uint8_t MIN_TWENTY_ELIDED_LEDS[]    = {138, 137, 136, 135};                          // "VENT"
inline constexpr uint8_t MIN_TWENTY_LEDS[]           = {138, 137, 136, 135, 134};                     // "VENTI"
inline constexpr uint8_t MIN_THIRTY_ELIDED_LEDS[]    = {133, 132, 131, 130, 129};                     // "TRENT"
inline constexpr uint8_t MIN_THIRTY_LEDS[]           = {133, 132, 131, 130, 129, 128};                // "TRENTA"
inline constexpr uint8_t MIN_FORTY_ELIDED_LEDS[]     = {119, 120, 121, 122, 123, 124, 125};           // "QUARANT"
inline constexpr uint8_t MIN_FORTY_LEDS[]            = {119, 120, 121, 122, 123, 124, 125, 126};      // "QUARANTA"
inline constexpr uint8_t MIN_FIFTY_ELIDED_LEDS[]     = {148, 149, 150, 151, 152, 153, 154, 155};      // "CINQUANT"
inline constexpr uint8_t MIN_FIFTY_LEDS[]            = {148, 149, 150, 151, 152, 153, 154, 155, 156}; // "CINQUANTA"

struct Tens {
  Word full;     // 20, 30, 40, 50
  Word elided;   // dropped trailing vowel, used when units ∈ {1, 8}
};

inline constexpr Tens TENS[4] = {
    {ORAQ_WORD(MIN_TWENTY_LEDS),  ORAQ_WORD(MIN_TWENTY_ELIDED_LEDS)},   // 20s
    {ORAQ_WORD(MIN_THIRTY_LEDS),  ORAQ_WORD(MIN_THIRTY_ELIDED_LEDS)},   // 30s
    {ORAQ_WORD(MIN_FORTY_LEDS),   ORAQ_WORD(MIN_FORTY_ELIDED_LEDS)},    // 40s
    {ORAQ_WORD(MIN_FIFTY_LEDS),   ORAQ_WORD(MIN_FIFTY_ELIDED_LEDS)},    // 50s
};

#undef ORAQ_WORD

}  // namespace it_words

// -----------------------------------------------------------------------------
// Italian grammar: composes the time as "SONO LE <hour> [E <minutes> MINUTI]".
// On the hour (mm == 00) only the hour word is shown, no connective, no suffix.
// -----------------------------------------------------------------------------
class ItalianLanguage final : public Language {
 public:
  const char *code() const override { return "it"; }
  const char *name() const override { return "Italian"; }
  int16_t heartbeat_led() const override { return it_words::HEARTBEAT_LED; }

  void render_time(Matrix &matrix, uint8_t hour, uint8_t minute,
                   Color color) const override {
    matrix.paint_word(it_words::PREFIX, color);
    matrix.paint_word(it_words::HOURS[hour], color);
    if (minute == 0) return;
    matrix.paint_word(it_words::AND_WORD, color);
    paint_minutes_(matrix, minute, color);
    matrix.paint_word(it_words::SUFFIX, color);
  }

  void render_phase(Matrix &matrix, uint8_t hour, uint8_t minute,
                    Color color, uint8_t phase) const override {
    switch (phase) {
      case 0: matrix.paint_word(it_words::PREFIX, color); break;
      case 1: matrix.paint_word(it_words::HOURS[hour], color); break;
      case 2: if (minute > 0) matrix.paint_word(it_words::AND_WORD, color); break;
      case 3: if (minute > 0) paint_minutes_(matrix, minute, color); break;
      case 4: if (minute > 0) matrix.paint_word(it_words::SUFFIX, color); break;
    }
  }

 private:
  void paint_minutes_(Matrix &matrix, uint8_t minute, Color color) const {
    if (minute < 20) {
      matrix.paint_word(it_words::MINUTE_UNITS[minute], color);
      return;
    }
    const uint8_t tens_idx = (minute / 10) - 2;   // 20s → 0, 30s → 1, 40s → 2, 50s → 3
    const uint8_t ones     = minute % 10;
    const bool elide       = (ones == 1 || ones == 8);
    const auto &t          = it_words::TENS[tens_idx];
    matrix.paint_word(elide ? t.elided : t.full, color);
    if (ones > 0) matrix.paint_word(it_words::MINUTE_UNITS[ones], color);
  }
};

}  // namespace oraquadra
}  // namespace esphome
