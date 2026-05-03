# OraQuadra → ESPHome

Italian word clock + BME680 + HA-driven effects, ported to ESPHome from the
original Arduino V1.2.9 sketch by Survival Hacking & contributors.

## Status

Working firmware:

- ESPHome YAML (Wi-Fi, API, OTA, captive portal, Improv, time)
- BME680 via `bme68x_bsec2` (temp/humidity/pressure/IAQ/CO2/VOC/accuracy)
- 256-LED matrix abstraction with serpentine xy↔led mapping
- Pluggable `Language` interface — Italian implementation
- IAQ frame ring (cornice tinted by air-quality bands)
- Universal blink-seconds (cornice second-hand pixel, hue rotates per second)
- 15 effect modes: Fade, Slow, Fast, Matrix, Matrix2, Tron, Moto, Galaga,
  Pacman, Digitale, Drop, Rainbow, Analog, Solid, Scroll
- HA notification queue with priority + JSON spec + 17-icon library
- HA services: `notify`, `scroll_banner`, `notify_warning`, `notify_info`
- HA entities for mode, color preset, brightness day/night, scroll text,
  scroll speed, sleep mode, blink seconds, IAQ frame, all-on smoke test

## Hardware

- ESP32-C3 (single-core RISC-V, USB nativo)
- 16×16 WS2812B matrix on **GPIO 4** — external 5 V supply required (the
  matrix can pull >9 A peak; USB will brown out)
- 1 button N.A. to GND on **GPIO 5**
- BME680 on **GPIO 3 (SDA) / GPIO 10 (SCL)** — moved off strapping pins
  (GPIO 0/1) which caused the i2c bus-recovery panic at boot. Standard
  4.7 kΩ pull-ups to 3.3 V on both lines.

## Files

```text
oraquadra-esphome/
├── oraquadra.yaml                      device config
├── secrets.yaml.example
└── components/oraquadra/               (flat — ESPHome external components
    ├── __init__.py                      do not recurse into subfolders)
    ├── oraquadra.{h,cpp}                main component
    ├── matrix.{h,cpp}                   16×16 + cornice abstraction
    ├── language.h                      pluggable Language interface
    ├── italian.h                       Italian vocab + grammar (one Language impl)
    ├── effect.h                        Effect base class
    ├── words_effect.h                  text time-of-day
    ├── analog_effect.h                 analog hands on cornice
    ├── notifications.{h,cpp}            HA notification queue + JSON
    ├── icons.{h,cpp}                    17 16×16 mono icons
    └── fonts.h                         5×7 ASCII font
```

## First boot

```bash
cp secrets.yaml.example secrets.yaml
# edit secrets.yaml
esphome run oraquadra.yaml
```

On first boot the device opens a Wi-Fi AP "OraQuadra Setup". Connect with a
phone, enter your network credentials, the device rejoins. After that, HA
auto-discovers it.

## HA services

Three native services exposed by the device — call from HA UI / automations
without building JSON.

```yaml
# Generic notification
service: esphome.oraquadra_notify
data:
  msg: "WARNING STORM"
  icon: "warning"
  color: "#ff0000"
  duration: 15
  priority: 3
  layout: "alternating"   # alternating | split | icon_only

# Quick warning (red, alert priority, 15 s, alternating)
service: esphome.oraquadra_notify_warning
data: { msg: "Door left open" }

# Quick info (default color, info priority, 10 s, alternating)
service: esphome.oraquadra_notify_info
data: { msg: "Bus arriving in 4 min" }

# Set scroll text + switch to scroll mode
service: esphome.oraquadra_scroll_banner
data: { msg: "BUON COMPLEANNO" }
```

Or push raw JSON to the `text.<node>_notification_in` entity:

```yaml
service: text.set_value
target:
  entity_id: text.oraquadra_notification_in
data:
  value: '{"text":"WARNING STORM","icon":"warning","color":"#ff0000","duration":15,"priority":3,"layout":"alternating"}'
```

Fields:

| Field | Type | Default | Notes |
| --- | --- | --- | --- |
| `text` | string | required | Up to ~30 chars renders cleanly |
| `icon` | string | `""` | Looked up in `icons.cpp` (warning, doorbell, …) |
| `color` | `#rrggbb` | `#ffffff` | Lowercase hex |
| `duration` | int (s) | `10` | Capped at `120` |
| `priority` | 0–3 | `1` | 0 = ambient, 3 = alert (preempts) |

Priority semantics:

- **3 — ALERT**: preempts current display immediately, repeats once
- **2 — WARNING**: head of queue
- **1 — INFO**: normal (default)
- **0 — AMBIENT**: dropped if anything higher is pending or active

Layout semantics:

- **alternating** (default): full-screen icon for 2 s, then full-screen
  scrolling text for the remainder
- **split**: 8×8 icon top-left + scrolling text in bottom 8 rows
- **icon_only**: just the 16×16 icon, no text

## HA automation examples

```yaml
# Doorbell → flash bell icon
automation:
  - alias: "OraQuadra: doorbell"
    trigger:
      platform: state
      entity_id: binary_sensor.doorbell
      to: "on"
    action:
      service: esphome.oraquadra_notify
      data:
        msg: "DING DONG"
        icon: "doorbell"
        color: "#ffaa00"
        duration: 8
        priority: 2
        layout: "alternating"

# Weather alert from HA notify integration
  - alias: "OraQuadra: weather warning"
    trigger:
      platform: state
      entity_id: sensor.weather_warning_level
      to: "severe"
    action:
      service: esphome.oraquadra_notify_warning
      data:
        msg: "{{ states('sensor.weather_warning_text') }}"

# Calendar event reminder
  - alias: "OraQuadra: calendar"
    trigger:
      platform: calendar
      event: start
      offset: "-00:05:00"
      entity_id: calendar.family
    action:
      service: esphome.oraquadra_scroll_banner
      data:
        msg: "{{ trigger.calendar_event.summary }}"
```

## Adding a new language

This grid is Italian-only (alphabet missing B and W; word placement tuned for
Italian time grammar). For a future build with a different faceplate:

1. Drop a new file under `components/oraquadra/languages/<lang>.h`
2. Add a vocabulary namespace (LED arrays + lookup tables)
3. Implement `class XxxLanguage final : public Language`
4. Swap `language_` in `OraquadraComponent::OraquadraComponent` (or expose as
   an HA select)

The rest of the firmware (effects, IAQ, notifications, analog clock) is
language-agnostic.

## License

GPL-3.0 — same as the original OraQuadra project.
