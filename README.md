# OraQuadra → ESPHome

Italian word clock + BME680 + HA-driven effects, ported to ESPHome from the
original Arduino V1.2.9 sketch by Survival Hacking & contributors.

## Status

Foundation in place. What works:

- ESPHome YAML skeleton (Wi-Fi, API, OTA, captive portal, Improv, time)
- BME680 via `bme68x_bsec2` (7 sensors auto-published to HA)
- 256-LED matrix abstraction with serpentine xy↔led mapping
- Pluggable `Language` interface — Italian implementation included
- `Words` effect (default text time-of-day)
- `Analog` effect (cornice-based hands)
- IAQ frame ring (cornice tinted by air-quality bands)
- HA notification queue with priority + JSON spec + 17-icon library
- HA entities for mode, color preset, brightness day/night, scroll text,
  notifications, switches

What's stubbed for the next pass (Phase 4 of the plan):

- 9 of 11 legacy effects (matrix-rain, tron, moto, galaga, pacman, drop,
  rainbow, fade, slow, digital overlay) — currently fall back to `Words`
- Scrolling text overlay during notifications (queue + icon flash works;
  text scroll on the right half is TBD)
- Day/night/sleep schedule wiring (entities exist, edge transitions TBD)

## Hardware

- ESP32-C3 (single-core RISC-V, USB nativo)
- 16×16 WS2812B matrix on **GPIO 4**
- 2× pulsanti N.A. to GND on **GPIO 6** (mode) and **GPIO 7** (color)
- BME680 on **GPIO 0 (SDA) / GPIO 1 (SCL)**
  - **External 2.2-4.7 kΩ pull-up to 3.3 V is required** on SDA — GPIO 0 is
    a strapping pin and a low level at reset prevents normal boot.

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

## HA notification spec

Push a notification by writing JSON to the `text.<node>_notification_in`
entity. Example HA service call:

```yaml
service: text.set_value
target:
  entity_id: text.oraquadra_notification_in
data:
  value: '{"text":"WARNING STORM","icon":"warning","color":"#ff0000","duration":15,"priority":3}'
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
