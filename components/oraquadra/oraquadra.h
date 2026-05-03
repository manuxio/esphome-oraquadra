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
#include "esphome/components/sensor/sensor.h"

#include "matrix.h"
#include "language.h"
#include "italian.h"
#include "notifications.h"
#include "effect.h"
#include "words_effect.h"
#include "analog_effect.h"

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
  void set_iaq_sensor(sensor::Sensor *s) { iaq_sensor_ = s; }
  void set_iaq_accuracy_sensor(sensor::Sensor *s) { iaq_accuracy_sensor_ = s; }

  // ---- HA-driven config (called from YAML lambdas) ------------------------
  void set_mode(uint8_t mode);
  void set_color_preset(uint8_t idx);
  void set_brightness_day(uint8_t b)   { brightness_day_ = b; apply_brightness_for_now(); }
  void set_brightness_night(uint8_t b) { brightness_night_ = b; apply_brightness_for_now(); }
  void set_scroll_speed_ms(uint16_t ms){ state_.scroll_speed_ms = ms; }
  void set_sleep_mode(bool on)         { sleep_mode_ = on; apply_brightness_for_now(); }
  void set_blink_seconds(bool on)      { state_.blink_seconds = on; }
  void set_iaq_frame(bool on)          { iaq_frame_enabled_ = on; }
  void set_scroll_text(const std::string &t) { scroll_text_ = t; }
  void set_iaq(float iaq, float accuracy);

  void push_notification_json(const std::string &json) { notifications_.enqueue_from_json(json); }

  // ---- Triggers from YAML ------------------------------------------------
  void boot_completed();
  void on_minute_change();
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

  Effect *current_effect_();
  Color iaq_color_for_(float iaq) const;
  bool is_night_now_() const;

  // ---- Wiring -------------------------------------------------------------
  // The YAML id references an AddressableLightState; we get the underlying
  // AddressableLight (the buffer) from it on setup().
  light::AddressableLightState *light_state_{nullptr};
  light::AddressableLight       *light_output_{nullptr};
  time::RealTimeClock           *time_{nullptr};
  sensor::Sensor          *iaq_sensor_{nullptr};
  sensor::Sensor          *iaq_accuracy_sensor_{nullptr};

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

  // ---- IAQ frame ----------------------------------------------------------
  bool iaq_frame_enabled_{true};
  float last_iaq_{0.0f};
  uint8_t last_iaq_accuracy_{0};

  // ---- Scrolling text -----------------------------------------------------
  std::string scroll_text_;

  // ---- Notifications ------------------------------------------------------
  NotificationQueue notifications_;
};

}  // namespace oraquadra
}  // namespace esphome
