# Service Alert System

A multi-part service alert platform combining a desktop/mobile app, an ESP32 VGA board based LCD, and a T-Watch S3 alert watch.

## Project overview

This repository contains three related components:

- `service-alerts-app` — Python-based GUI clients for alert monitoring and control.
- `service-alerts-device` — ESP32 base VGA board LCD display with LVGL and MQTT connectivity.
- `service-alerts-watch` — T-Watch S3 firmware for receiving alerts, displaying UI, and interacting with the alert system.

The system is built around MQTT topics such as `alert/button`, `alert/kitchen`, `alert/watchok`, and `alert/ads`.

## Components

### service-alerts-app

This folder contains two GUI prototypes:

- `pyqt_app.py` — a PyQt6-based cleaning alert monitor and MQTT client.
- `app.py` — a smaller Tkinter demo interface with a background image and custom button.

The PyQt application connects to MQTT, displays alert status, sounds alarms, and sends confirmations back to the alert network.

### service-alerts-device

This folder is an ESP-IDF project targeting RGB LCD panels and LVGL. It uses the Espressif example structure to drive a display, connect to WiFi, and handle MQTT events.

The device subscribes to alert notifications and `alert/ads`, and listens for `alert/watchok` to change the displayed state.

### service-alerts-watch

This folder contains Arduino-style firmware for a T-Watch S3 device. It uses:

- LVGL for touchscreen UI
- WiFi for network connectivity
- MQTT for alert messaging
- Light-sleep and RTC features to preserve power

The watch handles incoming `alert/button` messages, publishes `alert/watchok` for confirmation, and posts results to `alert/kitchen` when needed.

## Required software

### App

- Python 3.10+ (or compatible)
- PyQt6
- paho-mqtt
- Pillow

### Device

- ESP-IDF (matching the project requirements)
- A supported RGB LCD panel
- An ESP32-S3 or ESP32-P4 board

### Watch

- Arduino IDE / PlatformIO with ESP32 board support for T-Watch S3
- LilyGoLib, LVGL, PubSubClient, ArduinoJson, and related libraries

## Running the components

### 1. Desktop app

Install dependencies:

```bash
pip install pyqt6 paho-mqtt pillow
```

Run the main GUI app:

```bash
python service-alerts-app/pyqt_app.py
```

For the simpler demo version:

```bash
python service-alerts-app/app.py
```

### 2. ESP32 RGB LCD device

Change into the device folder:

```bash
cd service-alerts-device
```

Configure and build with ESP-IDF:

```bash
idf.py menuconfig
idf.py build flash monitor
```

Follow the existing `service-alerts-device/README.md` if you need RGB LCD panel wiring and LVGL buffer configuration guidance.

### 3. T-Watch firmware

Open `service-alerts-watch/cleaning_alert_watch.ino` in the Arduino IDE or your PlatformIO project.

Configure WiFi credentials and board settings as needed, then upload to the T-Watch device.

## MQTT topics used by the system

- `alert/button` — incoming alert message for the watch/device
- `alert/watchok` — watch confirmation that the alert was acknowledged
- `alert/kitchen` — kitchen or escalation status messages from the watch
- `alert/ads` — messages used by the device to switch display content or ads

## Notes

- The current source includes example MQTT broker settings and credentials. Replace these values before deploying to production.
- `service-alerts-device/README.md` is currently based on an ESP-IDF RGB LCD example and may be updated later to reflect the specific alert workflow.
- `service-alerts-watch` includes UI and power-management logic for the watch so it can operate as a networked alert responder.

## Contribution

If you want to extend this project, start by selecting one of the component folders and updating its README or source files. Pull requests should clarify which subsystem is being changed and how it affects MQTT message flow.

## License

No license is specified in this repository. Add a `LICENSE` file if you want to make the project open source.
