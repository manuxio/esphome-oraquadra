#include "notifications.h"
#include <algorithm>
#include <cstring>
#include "esphome/core/log.h"
#include "esphome/components/json/json_util.h"

namespace esphome {
namespace oraquadra {

static const char *const TAG = "oraquadra.notif";

// Parses "#rrggbb" or "#rgb" → Color. Returns white on malformed input.
static Color parse_hex_color(const std::string &s) {
  if (s.size() == 7 && s[0] == '#') {
    auto hx = [&](size_t i) -> uint8_t {
      char hi = s[i], lo = s[i + 1];
      auto v = [](char c) -> uint8_t {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return 10 + c - 'a';
        if (c >= 'A' && c <= 'F') return 10 + c - 'A';
        return 0;
      };
      return (v(hi) << 4) | v(lo);
    };
    return Color{hx(1), hx(3), hx(5)};
  }
  return Color{255, 255, 255};
}

bool NotificationQueue::enqueue_from_json(const std::string &payload) {
  if (payload.empty()) return false;

  Notification n;
  bool ok = json::parse_json(payload, [&](JsonObject root) -> bool {
    if (!root["text"].is<const char *>()) {
      ESP_LOGW(TAG, "notification JSON missing 'text'");
      return false;
    }
    n.text = root["text"] | "";
    n.icon_name = root["icon"] | "";
    if (root["color"].is<const char *>()) {
      n.color = parse_hex_color(root["color"] | "#ffffff");
    }
    uint16_t dur_s = root["duration"] | 10;
    if (dur_s > 120) dur_s = 120;
    n.duration_ms = static_cast<uint16_t>(dur_s * 1000);
    uint8_t pri = root["priority"] | 1;
    if (pri > 3) pri = 3;
    n.priority = static_cast<NotificationPriority>(pri);

    if (root["layout"].is<const char *>()) {
      const char *layout_str = root["layout"];
      if (std::strcmp(layout_str, "split") == 0) {
        n.layout = NotificationLayout::SPLIT;
      } else if (std::strcmp(layout_str, "icon_only") == 0) {
        n.layout = NotificationLayout::ICON_ONLY;
      } else if (std::strcmp(layout_str, "pixel_art") == 0) {
        n.layout = NotificationLayout::PIXEL_ART;
      } else {
        n.layout = NotificationLayout::ALTERNATING;  // default
      }
    }
    return true;
  });

  if (!ok) return false;

  // AMBIENT (0) is dropped if anything higher is already pending or active.
  if (n.priority == NotificationPriority::AMBIENT) {
    bool higher_pending = std::any_of(queue_.begin(), queue_.end(),
        [](const Notification &q) { return q.priority > NotificationPriority::AMBIENT; });
    if (higher_pending || (active_ && active_->priority > NotificationPriority::AMBIENT)) {
      ESP_LOGD(TAG, "dropped AMBIENT '%s' (higher priority already in flight)", n.text.c_str());
      return true;  // accepted but discarded
    }
  }

  // ALERT (3) preempts any active item and is scheduled to repeat once.
  if (n.priority == NotificationPriority::ALERT) {
    queue_.push_front(n);
    queue_.push_front(n);  // repeat once after the first run finishes
    if (active_) active_.reset();  // force restart of dispatch on next tick
    ESP_LOGW(TAG, "ALERT '%s' enqueued (preempting)", n.text.c_str());
    return true;
  }

  // Insertion sorted by priority (high → low), FIFO within priority.
  auto it = std::find_if(queue_.begin(), queue_.end(),
      [&](const Notification &q) { return q.priority < n.priority; });
  queue_.insert(it, std::move(n));
  ESP_LOGI(TAG, "queued '%s' pri=%u (depth=%u)",
           queue_.empty() ? "" : queue_.back().text.c_str(),
           static_cast<unsigned>(queue_.empty() ? 0 : (uint8_t) queue_.back().priority),
           static_cast<unsigned>(queue_.size()));
  return true;
}

void NotificationQueue::start_(Notification &&n, uint32_t now_ms) {
  active_ = std::move(n);
  active_started_ms_ = now_ms;
  ESP_LOGI(TAG, "showing '%s' for %u ms", active_->text.c_str(), active_->duration_ms);
}

const Notification *NotificationQueue::tick(uint32_t now_ms) {
  // Expire active.
  if (active_ && now_ms - active_started_ms_ >= active_->duration_ms) {
    active_.reset();
  }
  // Promote head if free.
  if (!active_ && !queue_.empty()) {
    Notification next = std::move(queue_.front());
    queue_.pop_front();
    start_(std::move(next), now_ms);
  }
  return active_ ? &*active_ : nullptr;
}

void NotificationQueue::clear() {
  queue_.clear();
  active_.reset();
}

}  // namespace oraquadra
}  // namespace esphome
