# rtcTrigger

`rtcTrigger` is an ESP32-based controller for small self-powered audio systems that need to wake on schedule, simulate button presses, and remain configurable over BLE.

The project currently targets:

- `Seeed XIAO ESP32C3` as the preferred hardware
- `DS3231` RTC modules over I2C with alarm wake via `SQW/INT`
- two PhotoMOS relays for `Power` and `Mode` button simulation
- a BLE web client and a native Capacitor app for Android/iPhone

The core use case is:

1. wake at an exact RTC time
2. hold the speaker power button
3. wait for boot
4. press mode once to switch away from Bluetooth

## Project Layout

- `src/`, `include/`: firmware
- `client/web/`: Web Bluetooth client for Chrome
- `client/app/`: Capacitor mobile app for Android and iPhone
- `platformio.ini`: board environments and firmware build config

## Main Features

- daily speaker schedule
- one-shot speaker trigger
- fixed BLE wake times through RTC
- interval BLE wake fallback
- battery voltage monitoring
- app-level BLE PIN protection
- per-board BLE name derived from MAC address
- trigger telemetry:
  - last event type
  - last event timestamp
  - speaker run counter

## Hardware

### Required

- ESP32 board
- DS3231 RTC module
- 2x PhotoMOS relay channels
- battery and regulator/power path appropriate for the board

### Preferred Board

- `Seeed XIAO ESP32C3`

Also supported for testing:

- `esp32-c3-devkitm-1`
- `Adafruit QT Py ESP32-S3`

## Default Firmware Targets

Defined in `platformio.ini`:

- `esp32-c3-devkitm-1`
- `seeed_xiao_esp32c3`
- `adafruit_qtpy_esp32s3_nopsram`

Build examples:

```bash
pio run -e seeed_xiao_esp32c3
pio run -e seeed_xiao_esp32c3 -t upload
pio device monitor --baud 115200
```

## Pin Mapping

### Seeed XIAO ESP32C3

Official pinout references:

Front view:

![Seeed XIAO ESP32C3 front pinout](https://files.seeedstudio.com/wiki/XIAO_WiFi/XIAO_ESP32-C3_front_pinout.png)

Back view:

![Seeed XIAO ESP32C3 back pinout](https://files.seeedstudio.com/wiki/XIAO_WiFi/XIAO_ESP32-C3_back_pinout.png)

Pins used by this project on the XIAO:

- `D1 / A1 / GPIO3`: Power relay
- `D2 / A2 / GPIO4`: Mode relay
- `D3 / GPIO5`: RTC `SQW/INT`
- `D4 / GPIO6`: I2C `SDA`
- `D5 / GPIO7`: I2C `SCL`
- `D0 / A0 / GPIO2`: battery ADC
- `D7 / GPIO20`: external status LED

Quick wiring table:

| Function | XIAO Pin | GPIO |
| --- | --- | --- |
| Power relay | `D1 / A1` | `GPIO3` |
| Mode relay | `D2 / A2` | `GPIO4` |
| RTC interrupt | `D3` | `GPIO5` |
| I2C SDA | `D4` | `GPIO6` |
| I2C SCL | `D5` | `GPIO7` |
| Battery ADC | `D0 / A0` | `GPIO2` |
| Status LED | `D7` | `GPIO20` |

- `GPIO3` / `D1` / `A1`: Power relay
- `GPIO4` / `D2` / `A2`: Mode relay
- `GPIO5` / `D3`: RTC `SQW/INT`
- `GPIO6` / `D4`: I2C `SDA`
- `GPIO7` / `D5`: I2C `SCL`
- `GPIO2` / `D0` / `A0`: battery ADC
- `GPIO20` / `D7`: external status LED
- `GPIO0`: recovery button / BOOT

### Adafruit QT Py ESP32-S3

- `GPIO18`: Power relay
- `GPIO17`: Mode relay
- `GPIO16`: RTC `SQW/INT`
- `GPIO41`: I2C `SDA`
- `GPIO40`: I2C `SCL`
- `GPIO9` / `A2`: battery ADC

## Wiring

### DS3231

Use:

- `VCC`
- `GND`
- `SDA`
- `SCL`
- `SQW`

Do not use:

- `32K`

Notes:

- `SQW/INT` is the alarm wake line
- it is effectively open-drain, so a pull-up may be needed depending on the module
- on the current setup, a pull-up to `3.3V` on `SQW` is acceptable if needed

### PhotoMOS Relays

Each relay input is driven from a GPIO through a series resistor.

Typical input wiring:

```text
GPIO -> 220R to 470R -> PhotoMOS input+
GND  ----------------> PhotoMOS input-
```

Output side:

- wire across the existing button pads
- use one relay for `Power`
- use one relay for `Mode`

### Battery Measurement

The firmware expects a divider ratio of about `2:1` by default.

Typical divider:

```text
Battery + ---- 100k ----+---- ADC pin
                        |
                       100k
                        |
Battery - --------------+---- GND
```

Default battery calibration:

- `divider x100 = 200`
- `empty = 3300 mV`
- `full = 4200 mV`

## Runtime Behavior

### Speaker Triggers

Supported trigger types:

- daily schedule
- one-shot
- direct BLE command

The current stable default timing is:

- `powerMs = 2200`
- `bootMs = 8000`
- `modeMs = 1000`

### BLE Wake Modes

Two BLE wake modes exist:

1. interval wake
   - controlled by `bleWakeSec`
2. fixed daily wake times
   - controlled by `bleFixedWake` and `bleWakeTimes`

Example fixed BLE wake times:

```text
09:00,13:00,17:00,19:00
```

### Recovery

Hold `BOOT` during reset/power-up to clear saved settings.

This restores defaults such as:

- PIN back to `123456`
- schedules
- battery calibration

## BLE Commands

The firmware uses a plain-text command protocol.

Common commands:

- `TURN_ON_NOW`
- `PRESS_POWER`
- `PRESS_MODE`
- `GET_STATUS`
- `A 123456`
- `PIN 987654`
- `SCH 08:00`
- `ONE 2026-05-29T19:15`
- `CLEAR_ONESHOT`
- `RTC 2026-05-29T19:15`
- `SET_TIMING 2200,8000,1000,10000,60`
- `BD 200`
- `BE 3300`
- `BF 4200`

Fixed BLE wake is sent internally in a compact form, but in the UI you enter normal times like:

```text
09:00,13:00,17:00,19:00
```

## Status / Telemetry

The live status includes:

- auth state
- one-shot state
- BLE wake mode and times
- battery percentage and voltage
- RTC time
- next trigger
- last event
- last event timestamp
- speaker run counter

Useful trigger verification fields:

- `lastEvent`
- `lastEventAt`
- `speakerRuns`

These are intended to let you verify scheduled execution even without a speaker connected.

## Web Client

Location:

- `client/web`

Supports:

- Chrome on macOS
- Chrome on Android

Local run example:

```bash
cd client/web
python3 -m http.server 8080
```

Then open:

```text
http://localhost:8080/client/web/
```

Notes:

- Android Chrome generally needs HTTPS for Web Bluetooth on non-local origins
- iPhone Safari does not support Web Bluetooth

## Native App

Location:

- `client/app`

The app is built with Capacitor and uses native BLE instead of Web Bluetooth.

Use Node 22 in that folder:

```bash
cd client/app
nvm use
npm install
```

Build web assets:

```bash
npm run build
```

Sync native projects:

```bash
npx cap sync android
npx cap sync ios
```

Open Android project:

```bash
npm run android
```

Open iOS project:

```bash
npm run ios
```

For iOS always use:

- `App.xcworkspace`

not:

- `App.xcodeproj`

## Git / Repo Notes

This project is intended to be tracked in git from the repository root.

Generated artifacts are ignored, including:

- `.pio/`
- `node_modules/`
- Android build products
- iOS Pods/build products

## Current Known State

At the time of this README rewrite, the following are working:

- RTC wake on `Seeed XIAO ESP32C3`
- one-shot triggers
- fixed BLE wake times
- BLE connection and PIN auth
- battery readout
- trigger telemetry persistence
- external status LED on `D7 / GPIO20`

## Notes

- If you want the previous README wording back, restore `README.original.md`
- If you want to change pin assignments, start with `include/settings.h`
- If you want to change the status LED pin on XIAO, edit `src/status_led.cpp`
