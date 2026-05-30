# Boom Remote Firmware

ESP32-C3 firmware for a battery-powered internal controller that:

- wakes on a `DS3231` RTC alarm for scheduled playback
- wakes periodically for a short BLE control window
- simulates the speaker `Power` and `Mode` buttons through two PhotoMOS outputs

## BLE Commands

Write plain-text commands to the command characteristic:

- `TURN_ON_NOW`
- `PRESS_POWER`
- `PRESS_MODE`
- `GET_STATUS`
- `AUTH 123456`
- `ENABLE_AUTH`
- `DISABLE_AUTH`
- `SET_PIN 987654`
- `ENABLE_SCHEDULE`
- `DISABLE_SCHEDULE`
- `SET_SCHEDULE HH:MM`
- `SET_ONESHOT YYYY-MM-DDTHH:MM`
- `CLEAR_ONESHOT`
- `SET_RTC YYYY-MM-DDTHH:MM`
- `SET_POWER_MS 2200`
- `SET_BOOT_MS 5000`
- `SET_MODE_MS 300`
- `SET_BLE_WINDOW_MS 10000`
- `SET_BLE_WAKE_SEC 60`
- `SET_TIMING 2200,5000,300,10000,60`
- `SET_BATTERY_DIVIDER_X100 200`
- `SET_BATTERY_EMPTY_MV 3300`
- `SET_BATTERY_FULL_MV 4200`

The status characteristic returns the current settings, battery percentage, and measured controller battery voltage.
It also reports the RTC time and the next scheduled trigger.

By default, BLE control commands are protected by a simple application PIN.
The default PIN is `123456` and should be changed from the web UI.

The web client also includes `Sync From Browser Time` to set the RTC from the current local browser time with one click.
The status panel shows battery percentage and voltage on a separate line.

## Recovery

If you forget the PIN or save a bad configuration, hold the board `BOOT` button while powering up or resetting the device.

This clears saved settings and restores defaults, including:

- PIN back to `123456`
- schedule settings
- battery calibration settings

The firmware also exposes the standard BLE Battery Service (`0x180F`) with Battery Level (`0x2A19`).

## Pin Defaults

- `GPIO6`: Power PhotoMOS control
- `GPIO7`: Mode PhotoMOS control
- `GPIO2`: DS3231 interrupt output
- `GPIO8`: I2C SDA
- `GPIO9`: I2C SCL
- `GPIO0`: battery ADC input

Update `include/settings.h` if your board wiring is different.

For the `Adafruit QT Py ESP32-S3` test target, the firmware automatically switches to:

- `GPIO18`: Power PhotoMOS control
- `GPIO17`: Mode PhotoMOS control
- `GPIO16`: DS3231 interrupt output
- `GPIO41`: I2C SDA
- `GPIO40`: I2C SCL
- `GPIO9` (`A2`): battery ADC input

For the `Seeed XIAO ESP32C3` target, the firmware automatically switches to:

- `GPIO3`: Power PhotoMOS control
- `GPIO4`: Mode PhotoMOS control
- `GPIO5`: DS3231 interrupt output
- `GPIO6`: I2C SDA
- `GPIO7`: I2C SCL
- `GPIO2` (`A0`-style analog use): battery ADC input

## Wiring Notes

- Use the PhotoMOS outputs as dry-contact closures across the existing speaker button pads.
- Connect the `DS3231` `SQW/INT` pin to `GPIO2`.
- The `DS3231` alarm line is active low and is used to wake the ESP32 from deep sleep.
- If you measure battery voltage, use a resistor divider and set the matching ratio with `SET_BATTERY_DIVIDER_X100`.

## Battery Calibration

- `batteryDividerRatioX100`: voltage divider scale factor times 100. Example: `200` means `2.00`.
- `batteryEmptyMv`: battery level reported as `0%`.
- `batteryFullMv`: battery level reported as `100%`.

For a typical `1S Li-ion` controller battery, a good starting point is:

- `SET_BATTERY_DIVIDER_X100 200`
- `SET_BATTERY_EMPTY_MV 3300`
- `SET_BATTERY_FULL_MV 4200`

## Build

```bash
pio run
pio run -t upload
pio device monitor
```

## Web Client

The folder `client/web` contains a small Web Bluetooth client for:

- Chrome on macOS
- Chrome on Android

Serve it locally, for example:

```bash
python3 -m http.server 8080
```

Then open `http://localhost:8080/client/web/` in Chrome.

Notes:

- The ESP32 advertises only during its BLE wake window.
- iPhone Safari does not support Web Bluetooth, so for iPhone use a generic BLE app such as `nRF Connect`.

### Android HTTPS Access

Android Chrome generally requires a secure HTTPS origin for Web Bluetooth. A simple LAN URL like `http://192.168.x.x:8080` is usually not enough.

The easiest path is a temporary HTTPS tunnel with a trusted certificate.

From `client/web` run:

```bash
npm run serve
```

In a second terminal run:

```bash
npm run tunnel
```

`localtunnel` will print a public `https://...` URL. Open that URL on Android Chrome and use the web client there.

Notes:

- keep both terminal windows running while testing
- allow Chrome `Nearby devices` and Bluetooth permissions on Android
- the ESP must be awake and advertising during the BLE window
