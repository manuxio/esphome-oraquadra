#pragma once

// =============================================================================
// HA notification queue.
//
// HA pushes JSON to a `text` entity; the component parses it into a
// Notification and calls `enqueue()`. The active notification preempts the
// inner 14×14 of the matrix for `duration_ms`, then the queue moves on.
//
// JSON shape:
//   { "text":"WARNING STORM",
//     "icon":"warning",
//     "color":"#ff0000",        (optional, defaults to white)
//     "duration":15,            (seconds, defaults to 10, capped at 120)
//     "priority":3 }            (0..3, defaults to 1)
//
// Priority ordering:
//   3 = ALERT     — preempts immediately, repeats once
//   2 = WARNING   — head of queue
//   1 = INFO      — normal
//   0 = AMBIENT   — dropped if any higher-priority item is pending
// =============================================================================

#include <cstdint>
#include <string>
#include <deque>
#include <optional>
#include "esphome/core/color.h"

namespace esphome {
namespace oraquadra {

enum class NotificationPriority : uint8_t {
  AMBIENT = 0,
  INFO    = 1,
  WARNING = 2,
  ALERT   = 3,
};

enum class NotificationLayout : uint8_t {
  // Alternates over time: icon → text scroll → icon. The default — most
  // visible because the icon catches attention then the message is read.
  ALTERNATING = 0,
  // Static side-by-side: 8x8 icon top-left + scroll text below. Compact and
  // simultaneous. Less impactful but message visible alongside icon.
  SPLIT       = 1,
  // Icon centered in matrix; text is NOT rendered (caller can put it in the
  // scroll_text entity instead). Lightest visual, useful for ambient alerts.
  ICON_ONLY   = 2,
  // Full-colour 16×16 pixel-art frame painted from the component's
  // pixel_art buffer (single slot — latest call wins). Set via
  // show_pixel_art() service.
  PIXEL_ART   = 3,
};

struct Notification {
  std::string text;
  std::string icon_name;
  Color color{255, 255, 255};
  uint16_t duration_ms{10000};
  NotificationPriority priority{NotificationPriority::INFO};
  NotificationLayout layout{NotificationLayout::ALTERNATING};
  uint32_t enqueued_ms{0};
};

class NotificationQueue {
 public:
  // Parses the JSON payload and enqueues one Notification. Returns true on
  // success. Bad JSON / missing fields are logged and ignored (no exceptions).
  bool enqueue_from_json(const std::string &json);

  // Promote head if a higher-priority item arrived; pop expired actives.
  // Call once per loop. Returns the currently-active notification (or
  // nullptr) — caller paints it if non-null.
  const Notification *tick(uint32_t now_ms);

  // Currently-rendered notification, or nullptr if none.
  const Notification *active() const { return active_ ? &*active_ : nullptr; }

  // millis() when the active notification started — needed by the renderer
  // because `Notification *` addresses are reused across optional<>::emplace
  // calls, so the renderer can't reliably detect "new notification" by ptr.
  uint32_t active_started_ms() const { return active_started_ms_; }

  void clear();
  size_t pending_count() const { return queue_.size(); }

 private:
  void start_(Notification &&n, uint32_t now_ms);

  std::deque<Notification> queue_;
  std::optional<Notification> active_;
  uint32_t active_started_ms_{0};
  bool alert_repeat_pending_{false};
};

}  // namespace oraquadra
}  // namespace esphome
