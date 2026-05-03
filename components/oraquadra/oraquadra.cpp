#include "oraquadra.h"
#include "icons.h"
#include "fonts.h"
#include "esphome/core/log.h"

namespace esphome {
namespace oraquadra {

static const char *const TAG = "oraquadra";

// Color presets exposed via the `select.<node>_color_preset` entity.
static const Color COLOR_PRESETS[] = {
    Color{255, 255, 255},  // 0 — White
    Color{  0,   0, 255},  // 1 — Blue
    Color{255,   0,   0},  // 2 — Red
    Color{  0, 255,   0},  // 3 — Green
    Color{255, 255,   0},  // 4 — Yellow
    Color{255, 165,   0},  // 5 — Orange
};
static constexpr uint8_t NUM_COLOR_PRESETS = sizeof(COLOR_PRESETS) / sizeof(COLOR_PRESETS[0]);

// IAQ → Color mapping for the cornice frame.
// Bands match the BME680 BSEC2 documentation.
Color OraquadraComponent::iaq_color_for_(float iaq) const {
  if (iaq <  50.f) return Color{ 0, 32,  0};   // excellent — dim green
  if (iaq < 100.f) return Color{20, 32,  0};   // good      — dim yellow-green
  if (iaq < 150.f) return Color{40, 32,  0};   // moderate  — dim yellow
  if (iaq < 200.f) return Color{48, 16,  0};   // poor      — dim orange
  if (iaq < 300.f) return Color{48,  0,  0};   // bad       — dim red
  return Color{32, 0, 32};                     // severe    — magenta (component pulses upstream)
}

void OraquadraComponent::setup() {
  ESP_LOGI(TAG, "OraQuadra setting up");

  if (light_state_ == nullptr) {
    ESP_LOGE(TAG, "no light_id wired — refusing to start");
    mark_failed();
    return;
  }

  // Get the addressable light output. AddressableLightState should override
  // get_output() to return AddressableLight*, but we defensively handle the
  // case where the cast yields null (e.g., output not yet wired, or actually
  // a non-addressable light).
  auto *raw_output = light_state_->get_output();
  if (raw_output == nullptr) {
    ESP_LOGE(TAG, "light_state->get_output() returned null — light not addressable?");
    mark_failed();
    return;
  }
  light_output_ = static_cast<light::AddressableLight *>(raw_output);
  if (light_output_ == nullptr) {
    ESP_LOGE(TAG, "light output is not an AddressableLight — refusing to start");
    mark_failed();
    return;
  }
  ESP_LOGI(TAG, "light output OK: %u pixels", light_output_->size());

  matrix_ = std::make_unique<Matrix>(light_output_);

  // Effect registry. Modes 3..11 (matrix, tron, pacman, etc.) are stubs for
  // now — they fall back to the Words effect until the V1.2.9 effects port
  // (Phase 4 of the plan) lands.
  effects_[MODE_FADE]    = std::make_unique<WordsEffect>();
  effects_[MODE_SLOW]    = std::make_unique<WordsEffect>();
  effects_[MODE_FAST]    = std::make_unique<WordsEffect>();
  effects_[MODE_MATRIX]  = std::make_unique<WordsEffect>();
  effects_[MODE_MATRIX2] = std::make_unique<WordsEffect>();
  effects_[MODE_TRON]    = std::make_unique<WordsEffect>();
  effects_[MODE_MOTO]    = std::make_unique<WordsEffect>();
  effects_[MODE_GALAGA]  = std::make_unique<WordsEffect>();
  effects_[MODE_PACMAN]  = std::make_unique<WordsEffect>();
  effects_[MODE_DIGITAL] = std::make_unique<WordsEffect>();
  effects_[MODE_DROP]    = std::make_unique<WordsEffect>();
  effects_[MODE_RAINBOW] = std::make_unique<WordsEffect>();
  effects_[MODE_ANALOG]  = std::make_unique<AnalogEffect>();

  state_.language = language_.get();
  state_.color    = COLOR_PRESETS[color_preset_idx_];
}

void OraquadraComponent::loop() {
  if (matrix_ == nullptr || time_ == nullptr) return;

  // Pull the wall clock once per loop.
  auto now = time_->now();
  if (!now.is_valid()) return;
  state_.hour   = now.hour;
  state_.minute = now.minute;
  state_.second = now.second;

  render_();
}

void OraquadraComponent::render_() {
  matrix_->clear();

  // Sleep mode → display stays dark.
  if (sleep_mode_) {
    matrix_->show();
    return;
  }

  // Layer 1: IAQ frame (cornice tinted by air quality).
  if (iaq_frame_enabled_ && last_iaq_accuracy_ > 0) {
    paint_iaq_base_layer_();
  }

  // Layer 2: notification or active effect (mutually exclusive on the inner matrix).
  const Notification *notif = notifications_.tick(millis());
  if (notif != nullptr) {
    paint_notification_(*notif);
  } else {
    Effect *effect = current_effect_();
    if (effect != nullptr) {
      effect->update(millis(), *matrix_, state_);
    }
  }

  matrix_->show();
}

void OraquadraComponent::paint_iaq_base_layer_() {
  Color base = iaq_color_for_(last_iaq_);
  matrix_->paint_cornice_uniform(base);
}

void OraquadraComponent::paint_notification_(const Notification &n) {
  // Icon in the inner-left 8 columns (rows 4..11 = vertically centered).
  const Icon *icon = find_icon(n.icon_name.c_str());
  if (icon != nullptr) {
    for (uint8_t row = 0; row < 16; row++) {
      uint16_t bits = icon->bitmap[row];
      for (uint8_t col = 0; col < 16; col++) {
        if (bits & (0x8000 >> col)) {
          matrix_->set_pixel(col, row, n.color);
        }
      }
    }
  }
  // Note: scroll-text rendering on the right half and timing animation are
  // wired in the next pass (kept simple here; the queue + icon flash is
  // already useful for HA-driven alerts).
}

Effect *OraquadraComponent::current_effect_() {
  if (current_mode_ >= NUM_MODES) return nullptr;
  return effects_[current_mode_].get();
}

void OraquadraComponent::set_mode(uint8_t mode) {
  if (mode >= NUM_MODES) {
    ESP_LOGW(TAG, "invalid mode %u", static_cast<unsigned>(mode));
    return;
  }
  current_mode_ = mode;
  if (auto *e = current_effect_()) {
    e->on_minute_change(state_.hour, state_.minute);
    ESP_LOGI(TAG, "mode → %s", e->name());
  }
}

void OraquadraComponent::set_color_preset(uint8_t idx) {
  if (idx >= NUM_COLOR_PRESETS) return;
  color_preset_idx_ = idx;
  state_.color = COLOR_PRESETS[idx];
}

void OraquadraComponent::set_iaq(float iaq) {
  last_iaq_ = iaq;
}

void OraquadraComponent::set_iaq_accuracy(float accuracy) {
  last_iaq_accuracy_ = static_cast<uint8_t>(accuracy);
}

void OraquadraComponent::on_minute_change() {
  if (auto *e = current_effect_()) e->on_minute_change(state_.hour, state_.minute);
}

bool OraquadraComponent::is_night_now_() const {
  // Simplistic: night = hour ∉ [7, 22). Replaced by HA `time:` triggers in
  // the YAML, which call apply_brightness_for_now() at the band edges.
  return state_.hour < 7 || state_.hour >= 22;
}

void OraquadraComponent::apply_brightness_for_now() {
  if (light_state_ == nullptr) return;
  uint8_t b = sleep_mode_ ? 0 : (is_night_now_() ? brightness_night_ : brightness_day_);
  auto call = light_state_->turn_on();
  call.set_brightness(b / 255.0f);
  call.perform();
}

void OraquadraComponent::boot_completed() {
  ESP_LOGI(TAG, "boot complete — language=%s, %u effects, %u icons",
           language_ ? language_->name() : "(none)",
           static_cast<unsigned>(effects_.size()),
           static_cast<unsigned>(ICONS_COUNT));
  apply_brightness_for_now();
}

// Button handlers — mirror the original .ino's button UX:
//   - mode short  → cycle preset/effect
//   - mode long   → toggle digital overlay (currently a no-op; wired later)
//   - color short → cycle color preset
//   - color long  → toggle blink-seconds
void OraquadraComponent::on_btn_mode_short() {
  set_mode((current_mode_ + 1) % NUM_MODES);
}
void OraquadraComponent::on_btn_mode_long() {
  ESP_LOGI(TAG, "btn-mode long: digital overlay toggle (TODO)");
}
void OraquadraComponent::on_btn_sec_short() {
  set_color_preset((color_preset_idx_ + 1) % NUM_COLOR_PRESETS);
}
void OraquadraComponent::on_btn_sec_long() {
  state_.blink_seconds = !state_.blink_seconds;
  ESP_LOGI(TAG, "blink seconds → %s", state_.blink_seconds ? "on" : "off");
}

void OraquadraComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "OraQuadra:");
  ESP_LOGCONFIG(TAG, "  Language: %s (%s)",
                language_ ? language_->name() : "?",
                language_ ? language_->code() : "?");
  ESP_LOGCONFIG(TAG, "  Effects:  %u", static_cast<unsigned>(effects_.size()));
  ESP_LOGCONFIG(TAG, "  Icons:    %u", static_cast<unsigned>(ICONS_COUNT));
  ESP_LOGCONFIG(TAG, "  Brightness: day=%u night=%u sleep=%s",
                brightness_day_, brightness_night_, sleep_mode_ ? "yes" : "no");
}

}  // namespace oraquadra
}  // namespace esphome
