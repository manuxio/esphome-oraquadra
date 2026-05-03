#pragma once

// =============================================================================
// OraQuadra main component.
//
// Owns the render pipeline and routes HA-driven config (mode, color,
// brightness, scroll text, notifications, IAQ frame, …) into the matrix.
//
// Render pipeline, executed once per loop tick:
//
//   1. Update wall-clock state (hour, minute, second) from the time source.
//   2. Clear the matrix buffer.
//   3. Paint IAQ base layer on the cornice (if enabled).
//   4. Run active Effect (or nothing if a notification is preempting).
//   5. Overlay active notification (icon + scroll text), if any.
//   6. Commit to the LED strip.
// =============================================================================

#include <array>
#include <memory>
#include "esphome/core/component.h"
#include "esphome/core/color.h"
#include "esphome/components/light/addressable_light.h"
#include "esphome/components/time/real_time_clock.h"

#include "matrix.h"
#include "language.h"
#include "italian.h"
#include "notifications.h"
#include "effect.h"
#include "words_effect.h"
#include "analog_effect.h"
#include "solid_effect.h"
#include "rainbow_effect.h"
#include "slow_effect.h"
#include "fade_effect.h"
#include "matrix_effect.h"
#include "matrix2_effect.h"
#include "drop_effect.h"
#include "tron_effect.h"
#include "moto_effect.h"
#include "galaga_effect.h"
#include "pacman_effect.h"
#include "digital_effect.h"
#include "scroll_effect.h"
#include "diag_effect.h"

namespace esphome {
namespace oraquadra {

enum Mode : uint8_t {
  MODE_FADE     = 0,
  MODE_SLOW     = 1,
  MODE_FAST     = 2,
  MODE_MATRIX   = 3,
  MODE_MATRIX2  = 4,
  MODE_TRON     = 5,
  MODE_MOTO     = 6,
  MODE_GALAGA   = 7,
  MODE_PACMAN   = 8,
  MODE_DIGITAL  = 9,
  MODE_DROP     = 10,
  MODE_RAINBOW  = 11,
  MODE_ANALOG   = 12,
  MODE_SOLID    = 13,   // fill matrix with current color (HA "all on" use case)
  MODE_SCROLL   = 14,   // scroll the user-configured text continuously
  MODE_DIAG     = 15,   // static test pattern for data-line debugging
  NUM_MODES
};

class OraquadraComponent : public Component {
 public:
  // ---- ESPHome lifecycle --------------------------------------------------
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

  // ---- Wiring (called from codegen / __init__.py) -------------------------
  void set_light(light::AddressableLightState *l) { light_state_ = l; }
  void set_time(time::RealTimeClock *t) { time_ = t; }
  // IAQ values arrive via set_iaq() from the BME680's on_value lambda — no
  // direct sensor pointer needed (avoids a circular codegen dependency).

  // ---- HA-driven config (called from YAML lambdas) ------------------------
  void set_mode(uint8_t mode);
  void cycle_mode() { set_mode((current_mode_ + 1) % NUM_MODES); }
  void set_color_preset(uint8_t idx);
  // Smoke-test action: full white for 5 s, then auto-revert to whatever mode
  // was active before. Wiring is in HA via a button entity.
  void all_on();
  // Brightness setters are now safe to call any time — we don't touch the
  // LightState anymore (which used to crash during early restore). Brightness
  // is applied per-pixel at the end of every render via current_brightness_().
  void set_brightness_day(uint8_t b)   { brightness_day_ = b; }
  void set_brightness_night(uint8_t b) { brightness_night_ = b; }
  // 0..100 % of current global brightness applied to IAQ-frame pixels only.
  // Lets the user dim the cornice ring without dimming the whole display
  // (and reduces continuous current draw from 60 saturated cornice LEDs).
  void set_iaq_brightness_pct(uint8_t pct) {
    iaq_brightness_pct_ = (pct > 100) ? 100 : pct;
  }
  void set_scroll_speed_ms(uint16_t ms){ state_.scroll_speed_ms = ms; }
  void set_sleep_mode(bool on)         { sleep_mode_ = on; }
  void set_blink_seconds(bool on)      { state_.blink_seconds = on; }
  void set_iaq_frame(bool on)          { iaq_frame_enabled_ = on; }
  void set_scroll_text(const std::string &t);
  // Convenience: HA pushes a one-shot notification. Used by the api.services
  // so HA can call it directly with arguments instead of building JSON.
  void notify(const std::string &text, const std::string &icon,
              const std::string &color, int duration, int priority,
              const std::string &layout);
  // Two separate setters so the YAML doesn't need a cross-sensor reference
  // (which would create a circular codegen dependency inside the BME680
  // sensor block).
  void set_iaq(float iaq);
  void set_iaq_accuracy(float accuracy);

  void push_notification_json(const std::string &json) { notifications_.enqueue_from_json(json); }

  // ---- Triggers from YAML ------------------------------------------------
  void boot_completed();
  void on_minute_change();
  // Called from the addressable_lambda hook on every strip update tick.
  // Copies the shadow buffer into the strip's live buffer; this is the
  // only time the strip buffer is touched, so it cannot race with the
  // RMT transmission.
  void flush_strip() { if (matrix_) matrix_->flush_to_strip(); }
  void apply_brightness_for_now();
  void on_btn_mode_short();
  void on_btn_mode_long();
  void on_btn_sec_short();
  void on_btn_sec_long();

 protected:
  // Render pipeline steps.
  void render_();
  void paint_iaq_base_layer_();
  void paint_notification_(const Notification &n);
  void paint_seconds_hand_();

  // Notification-layout helpers.
  void paint_notif_alternating_(const Notification &n, uint32_t elapsed_ms);
  void paint_notif_split_(const Notification &n, uint32_t elapsed_ms);
  void paint_notif_icon_only_(const Notification &n);
  void paint_icon_full_(const char *icon_name, Color c);
  void paint_icon_8_(const char *icon_name, uint8_t x0, uint8_t y0, Color c);
  void paint_scroll_text_full_(const char *text, Color c, uint32_t scroll_ms);
  void paint_scroll_text_band_(const char *text, uint8_t y0, Color c, uint32_t scroll_ms);

  Effect *current_effect_();
  Color iaq_color_for_(float iaq) const;
  bool is_night_now_() const;
  uint8_t current_brightness_() const;

  // Tracks when the currently-active notification became active, for
  // alternating layout timing.
  uint32_t active_notification_started_ms_{0};

  // ---- Wiring -------------------------------------------------------------
  // The YAML id references an AddressableLightState; we get the underlying
  // AddressableLight (the buffer) from it on setup().
  light::AddressableLightState *light_state_{nullptr};
  light::AddressableLight       *light_output_{nullptr};
  time::RealTimeClock           *time_{nullptr};

  // ---- Geometry / IO ------------------------------------------------------
  std::unique_ptr<Matrix> matrix_;

  // ---- Language (only Italian for V1.2.9 hardware; pluggable for future) --
  std::unique_ptr<Language> language_{std::make_unique<ItalianLanguage>()};

  // ---- Effect registry ----------------------------------------------------
  std::array<std::unique_ptr<Effect>, NUM_MODES> effects_{};

  // ---- Per-tick state -----------------------------------------------------
  ClockState state_{};

  // ---- Mode / colors ------------------------------------------------------
  uint8_t current_mode_{MODE_FAST};
  uint8_t color_preset_idx_{0};

  // ---- Brightness schedule ------------------------------------------------
  uint8_t brightness_day_{204};
  uint8_t brightness_night_{26};
  bool sleep_mode_{false};

  // True after on_boot fires (priority -100, post-setup of every component).
  // Until then, apply_brightness_for_now() is unsafe because LightState's
  // output_ may not be wired yet.
  bool boot_done_{false};

  // ---- IAQ frame ----------------------------------------------------------
  bool iaq_frame_enabled_{true};
  float last_iaq_{0.0f};
  uint8_t last_iaq_accuracy_{0};
  // 0..100 % of the current global brightness applied to cornice IAQ pixels.
  // Defaults to 30: the ring is informative but doesn't dominate the display
  // and keeps the cornice current draw down (less voltage dip on long data
  // lines, which manifested as random pixel flicker on this hardware).
  uint8_t iaq_brightness_pct_{30};

  // ---- Scrolling text -----------------------------------------------------
  std::string scroll_text_;

  // ---- Smoke-test (all_on) ------------------------------------------------
  // When the HA "All On" button is pressed we switch to MODE_SOLID + white
  // for 5 s, then auto-restore the previous mode/color so the user doesn't
  // get stuck looking at a fully-lit matrix.
  uint32_t all_on_revert_at_ms_{0};
  uint8_t  saved_mode_{MODE_FAST};
  uint8_t  saved_color_idx_{0};

  // Mode the user was on before they typed text into the scroll-text input
  // (we auto-switch to MODE_SCROLL on non-empty text and revert on clear).
  uint8_t  saved_mode_before_scroll_{MODE_FAST};

  // ---- Notifications ------------------------------------------------------
  NotificationQueue notifications_;
};

}  // namespace oraquadra
}  // namespace esphome
