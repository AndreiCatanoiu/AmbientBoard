# AmbientBoard

AmbientBoard is an ESP32-based ambient display panel: it shows NTP-synchronized time and date, reads temperature and humidity from a DHT sensor, provides a touch interface built with LVGL, connects to WiFi, publishes and receives messages over MQTT, and updates its firmware via secure OTA (HTTPS). The device is designed as a connected desk or home gadget with a configurable RGB LED on the back and a resistive-touch TFT screen.

---

## Table of Contents

1. [Features](#features)
2. [Required Hardware](#required-hardware)
3. [Pinout](#pinout)
4. [Software Stack](#software-stack)
5. [Architecture](#architecture)
6. [Project Structure](#project-structure)
7. [Core Modules](#core-modules)
8. [User Interface](#user-interface)
9. [Connectivity](#connectivity)
10. [OTA Updates](#ota-updates)
11. [Local Secrets Setup](#local-secrets-setup)
12. [Installation and Build](#installation-and-build)
13. [Flash and Serial Monitor](#flash-and-serial-monitor)
14. [Flash Partitions](#flash-partitions)
15. [Power Management and Sleep](#power-management-and-sleep)
16. [Troubleshooting](#troubleshooting)
17. [License](#license)

---

## Features

| Area | Details |
|------|---------|
| Display | TFT 240x320 (ST7789/ILI9341 driver), SPI, controllable backlight |
| Touch | XPT2046 resistive controller, separate SPI bus |
| Sensor | DHT (temperature + humidity) on GPIO 27, with median filtering |
| LED | RGB on GPIO 4, 16, 17; color stored in NVS |
| Time | NTP sync (`pool.ntp.org`), Romania timezone (`EET-2EEST`) |
| WiFi | Scan, connect, credential storage in NVS, auto-reconnect |
| MQTT | TLS (MQTTS), topic `ambientboard/<username>`, authentication |
| Messages | HTTPS inbox pull; outbound messages via MQTT publish |
| OTA | Version check from `latest.json`, HTTPS download, dual-slot |
| UI | LVGL 8.4: Home, Messages, Temperature, Settings, Calendar |
| Themes | Configurable color palette, persisted in NVS |
| Sleep | Backlight and LED turn off after 3 minutes of inactivity |

---

## Required Hardware

- **ESP32 Dev Module** board (4 MB flash)
- **240x320** TFT display compatible with ILI9341 / ST7789
- **XPT2046** touch controller
- **DHT11** or **DHT22** sensor (single-GPIO bit-banging implementation)
- RGB LED (common cathode or with resistors) on 3 GPIO pins
- Stable 5 V / 3.3 V power supply (higher draw with backlight and active WiFi)

The wiring is defined in the driver headers under `lib/display/` and `lib/rgb_led/`.

---

## Pinout

### TFT Display (SPI)

| Signal | GPIO |
|--------|------|
| MOSI   | 13   |
| MISO   | 12   |
| SCLK   | 14   |
| CS     | 15   |
| DC     | 2    |
| Backlight | 21 |

Resolution: **240 x 320**. The `DISPLAY_USE_ST7789` macro controls orientation and color inversion for ST7789 vs ILI9341 modules.

### XPT2046 Touch (SPI)

| Signal | GPIO |
|--------|------|
| MOSI   | 32   |
| MISO   | 39   |
| SCLK   | 25   |
| CS     | 33   |
| IRQ    | 36   |

Touch calibration: `XPT2046_INVERT_X`, `XPT2046_RAW_MIN/MAX` in `lib/display/xpt2046.h`.

### RGB LED

| Channel | GPIO |
|---------|------|
| R       | 4    |
| G       | 16   |
| B       | 17   |

### Temperature Sensor

| Signal | GPIO |
|--------|------|
| Data   | 27   |

---

## Software Stack

| Component | Version / detail |
|-----------|-----------------|
| PlatformIO | `espressif32` platform |
| Board      | `esp32dev` |
| Framework  | ESP-IDF 5.3 |
| UI         | LVGL 8.4 (`lvgl/lvgl@^8.4.0`) |
| RTOS       | FreeRTOS (dual-core ESP32) |
| Build      | CMake + Ninja |

The current firmware version is defined at build time via the `CURRENT_FW_VERSION` macro in `platformio.ini` (e.g. `"v1.1.0"`).

---

## Architecture

```
                    +------------------+
                    |     app_main     |
                    +--------+---------+
                             |
     +-----------+-----------+-----------+-----------+
     |           |           |           |           |
     v           v           v           v           v
 settings    app_state    display       ui        rgb_led
  _store                  + LVGL
     |           ^           |           |
     |           |           |           |
     +-----+-----+-----+-----+-----+-----+
           |           |           |
           v           v           v
        wifi       time_mgr    temp_sensor
           |                       |
           v                       |
         mqtt <--------------------+
           |
           v
        inbox (HTTPS pull)

     FreeRTOS tasks:
     - display_task  (priority 4, stack 12 KB) -- LVGL handler + sleep
     - wifi_task     (priority 5, stack 4 KB)
     - time_task     (priority 5, stack 4 KB)
     - temp_sensor_task (priority 5, core 1, stack 4 KB)
     - ota_ver_task / ota_task (on demand, stack 8 KB)
     - inbox_pull    (on demand, stack 6 KB)
```

**Centralized data model:** the `app_state` module holds the current sensor, time, and WiFi state. Background tasks write to `app_state`; LVGL screens read from it periodically via timers.

**LVGL synchronization:** a recursive mutex (`display_lock` / `display_unlock`) protects `lv_timer_handler()` calls inside `display_task`.

---

## Project Structure

```
AmbientBoard/
|-- src/
|   |-- main.c                 # Entry point, module init, task startup
|   |-- CMakeLists.txt
|
|-- lib/
|   |-- app_state/             # Shared sensor / time / WiFi state
|   |-- display/               # ILI9341, XPT2046, LVGL port, backlight sleep
|   |-- ui/                    # LVGL screens, router, theme, on-screen keyboard
|   |-- wifi/                  # WiFi STA manager
|   |-- time/                  # NTP + Romania timezone
|   |-- senzor_temperatura/    # DHT bit-banging driver
|   |-- rgb_led/               # RGB LED control + suspend/resume
|   |-- storage/               # NVS: WiFi, LED, theme, calendar, MQTT name
|   |-- mqtt/                  # MQTTS client
|   |-- inbox/                 # HTTPS message pull (local file, gitignored)
|   |-- ota/                   # Version check + HTTPS OTA
|   |-- remote/                # Server config (host, URIs, credentials)
|   |-- lv_conf/               # LVGL configuration
|
|-- partitions.csv             # Dual-OTA partition table
|-- platformio.ini
|-- sdkconfig.defaults
|-- sdkconfig.esp32dev
|-- disable_component_manager.py
|-- LICENSE
```

Sensitive files (certificates, credentials, inbox logic) are excluded from git; `.example` templates are provided.

---

## Core Modules

### `app_state`

Shared structures between tasks and the UI:

- `app_sensor_t` -- temperature (tenths, sign), humidity (tenths), valid flag
- `app_time_t` -- time, date, NTP synced flag
- `app_wifi_t` -- connected, signal strength, IP, SSID

### `display`

- Initializes SPI display + touch
- Registers LVGL driver (flush callback, pointer input)
- 2 ms ESP timer for `lv_tick_inc`
- **Sleep:** after 180 seconds without touch, turns off backlight and suspends the LED
- **Wake:** first touch wakes the backlight and re-applies the LED color from NVS

### `settings_store`

NVS (Non-Volatile Storage) persistence:

- WiFi credentials (SSID + password)
- LED color (RGB packed in uint32)
- UI theme index
- MQTT device name (max 20 characters)

### `wifi_manager`

- STA mode, network scan, manual connection from the UI
- Automatic credential storage in NVS
- RSSI to signal percentage mapping (0--100%)
- Updates `app_state` with IP and SSID

### `time_manager`

- NTP server: `pool.ntp.org`
- Timezone: `EET-2EEST,M3.5.0/3,M10.5.0/4` (Romania)
- Periodic task that updates `app_state`

### `senzor_temperatura`

- DHT protocol on GPIO 27 (bit-banging with CPU cycle measurement)
- Median filter over the last 5 samples
- Outlier rejection (delta > 2.5 degrees C)
- Read interval: 3 seconds
- Task pinned to core 1

### `mqtt_comm`

- MQTTS connection to the broker configured in `remote_secret.c`
- CA certificate in `lib/mqtt/mqtt_ca_pem.h` (gitignored)
- Client ID: `amb_<name>` or `amb_<partial MAC>` if no name is set
- Publish to topic `ambientboard/<name>` with QoS 1
- Reconnect when the name is changed in Settings

### `inbox`

- Async message pull from an HTTPS URL (`messages.json`)
- Simple JSON parsing (no dedicated library)
- Max 10 messages in memory; each with sender + text
- Refresh on MQTT connect and periodically from the Messages screen

### `ota_update`

- Checks `https://<host>/<ota_path>/latest.json`
- Expected format: `{"version": "v1.2.0", "bin": "firmware_v1.2.0.bin"}`
- Semver-like version comparison (major.minor.patch)
- Binary download via `esp_https_ota` with ESP certificate bundle
- Automatic reboot after a successful update
- States: IDLE, CHECKING, DOWNLOADING, DONE, FAILED

### `remote_cfg` / `remote_secret`

- Abstracts host, MQTT port, user/password, OTA URL, inbox pull URL
- Actual values live in `remote_secret.c` (copied from `.example`, gitignored)
- Sensitive strings do not appear in versioned code

---

## User Interface

Navigation is handled by `ui_router`: the Home screen stays persistent; other screens are created on demand and destroyed on return (slide-left animation).

### Home

- Large clock (font 48), weekday, date
- 2x2 grid with quick access: Messages, Temperature, Settings, Calendar
- Refreshed every 500 ms from `app_state.time`

### Messages

- Message list from inbox (HTTPS pull)
- MQTT broker connection status
- Compose button (on-screen keyboard) -- active only with WiFi + name set + MQTT connected
- Inbox refresh every ~20 seconds when WiFi is active

### Temperature

- Temperature and humidity display from `app_state.sensor`
- Refreshed every 1 second

### Settings

| Section | Functionality |
|---------|---------------|
| Color theme | Selection from a predefined palette |
| LED | Modal with R/G/B sliders + preview |
| Username | On-screen keyboard; used in the MQTT topic |
| WiFi | Scan, connect, disconnect, reconnect |
| Firmware | Current version, latest from server, update button if a newer version exists |
| System info | Chip, ESP-IDF, MAC, free heap, uptime |

### Calendar

- LVGL calendar widget
- Initial date from NTP-synced time (fallback 2026-01-01)
- Events can be stored in NVS (API in `settings_store`)

### On-screen keyboard (`ui_keyboard`)

- Used for WiFi password, MQTT name, message composition
- Overlay on `lv_layer_top`

---

## Connectivity

### WiFi

1. On first boot, the user scans networks from Settings
2. Selects a network; if secured, enters the password
3. Credentials are saved to NVS
4. On reboot, automatic reconnection

### MQTT

```
Broker:  mqtts://<host>:8883
Auth:    username + password (from remote_secret.c)
Topic:   ambientboard/<username>
Client:  amb_<username>
TLS:     CA certificate in mqtt_ca_pem.h
```

Messages sent by the device appear on the topic above. The inbox pulls messages from other users via the configured HTTPS server.

---

## OTA Updates

### Flow

1. When the Settings screen opens, `ota_check_version()` runs in the background
2. `latest.json` is downloaded from the server
3. The UI shows `Firmware: <current>` and `Latest: <server>`
4. If the server version is newer, the **Install update** button appears
5. On press, `ota_update_start()` downloads the binary and writes it to the inactive OTA slot
6. Automatic reboot into the new firmware

### `latest.json` format

```json
{
  "version": "v1.2.0",
  "bin": "AmbientBoard_v1.2.0.bin"
}
```

Binary name validation rules:

- Must not contain `/`, `\`, or `..`
- Must not be empty

---

## Local Secrets Setup

Before the first functional build, copy and fill in the gitignored files:

### 1. `lib/remote/remote_secret.c`

```bash
cp lib/remote/remote_secret.c.example lib/remote/remote_secret.c
```

Fill in:

- `HOST` -- server domain
- `OTA_PATH` -- HTTPS path for firmware (e.g. `/ota-xxxxx`)
- `PULL_PATH` -- HTTPS path for inbox JSON (e.g. `/ota-xxxxx/messages.json`)
- `MQTT_USER`, `MQTT_PASS`, `MQTT_PORT` (default 8883)

### 2. `lib/mqtt/mqtt_ca_pem.h`

Create the file with the MQTT broker CA certificate:

```c
#pragma once
static const char MQTT_CA_CERT_PEM[] =
"-----BEGIN CERTIFICATE-----\n"
"..."
"-----END CERTIFICATE-----\n";
```

### 3. `lib/inbox/inbox.c`

```bash
cp lib/inbox/inbox.c.example lib/inbox/inbox.c
```

Pull logic is already implemented in the example file; no changes are needed if the URL in `remote_secret.c` is correct.

---

## Installation and Build

### Requirements

- [PlatformIO Core](https://platformio.org/) or the PlatformIO extension for VS Code / Cursor
- USB-UART driver for ESP32 (CP2102 / CH340)
- Git

### Steps

```bash
# Clone the repository
git clone <repo-url> AmbientBoard
cd AmbientBoard

# Switch to the branch with the full implementation
git checkout dev

# Configure secrets (see section above)
cp lib/remote/remote_secret.c.example lib/remote/remote_secret.c
cp lib/inbox/inbox.c.example lib/inbox/inbox.c
# Edit remote_secret.c and create mqtt_ca_pem.h

# Build
pio run
```

### Relevant `platformio.ini` options

| Option | Value | Role |
|--------|-------|------|
| `upload_port` | COM3 | Serial port (adjust for your system) |
| `upload_speed` | 921600 | Flash speed |
| `monitor_speed` | 115200 | Serial monitor speed |
| `board_upload.flash_size` | 4MB | Flash size |
| `board_build.partitions` | partitions.csv | Custom partition table |
| `extra_scripts` | disable_component_manager.py | Disables IDF Component Manager |
| `lib_deps` | lvgl/lvgl@^8.4.0 | UI library |
| `build_flags` | CURRENT_FW_VERSION, LV_CONF | Compile-time macros |

The `disable_component_manager.py` script sets `IDF_COMPONENT_MANAGER=0` to avoid CMake conflicts between PlatformIO LVGL and the IDF registry.

---

## Flash and Serial Monitor

```bash
# Upload firmware
pio run -t upload

# Serial monitor
pio device monitor

# Upload + monitor
pio run -t upload && pio device monitor

# Full build clean (for CMake errors)
pio run -t fullclean
pio run
```

Adjust `upload_port` in `platformio.ini` to your board's COM / `/dev/ttyUSB*` port.

---

## Flash Partitions

`partitions.csv`:

| Partition | Type  | Offset   | Size     |
|-----------|-------|----------|----------|
| nvs       | data  | 0x9000   | 16 KB    |
| otadata   | data  | 0xD000   | 8 KB     |
| phy_init  | data  | 0xF000   | 4 KB     |
| ota_0     | app   | 0x10000  | 1.875 MB |
| ota_1     | app   | 0x1F0000 | 1.875 MB |

Dual-slot OTA: the active and backup firmware each occupy ~1.9 MB. NVS preserves user settings across updates.

---

## Power Management and Sleep

| Parameter | Value |
|-----------|-------|
| Inactivity timeout | 180 seconds (3 minutes) |
| Sleep action | Backlight OFF + `rgb_led_suspend()` |
| Wake | Screen touch (first touch is not processed as LVGL input) |
| On wake | Backlight ON + `rgb_led_resume()` (re-applies color from NVS) |

Any valid touch while the screen is active resets the inactivity timer.

---

## Troubleshooting

| Problem | Likely cause | Fix |
|---------|--------------|-----|
| Black screen | Backlight sleep or SPI wiring | Touch the screen; check GPIO 21 and SPI |
| Inverted touch | Wrong calibration | Adjust `XPT2046_INVERT_X/Y` and raw min/max |
| WiFi won't connect | Wrong credentials | Re-scan and reconnect from Settings |
| MQTT disconnected | Wrong CA or credentials | Check `mqtt_ca_pem.h` and `remote_secret.c` |
| OTA failed | No WiFi or invalid manifest | Connect WiFi; verify `latest.json` on server |
| CMake / target-erase_flash | Corrupt PlatformIO cache | `pio run -t fullclean` then rebuild |
| Temperature `--` | Sensor disconnected or wrong GPIO | Check DHT on GPIO 27 |
| Empty messages | Wrong inbox URL or no WiFi | Check `PULL_PATH` and connectivity |

ESP-IDF logs are available on the serial monitor at 115200 baud. Useful tags: `APP_MAIN`, `DISPLAY`, `WIFI_MGR`, `MQTT`, `OTA`, `TEMP_SENSOR`, `INBOX`.

---

## License

This project is distributed under the **MIT** license. See [LICENSE](LICENSE).