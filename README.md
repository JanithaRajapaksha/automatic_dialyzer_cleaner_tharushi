# Automatic Dialyzer Cleaner - Embedded Control System

Modular ESP32 firmware running FreeRTOS for a 10-channel Automated Dialyzer Cleaner controller, built for **PlatformIO**.

---

## 📁 PlatformIO Project Structure

```
automatic_dialyzer_cleaner_tharushi/
├── platformio.ini       # PlatformIO project configuration & environment
├── include/             # Header files
│   ├── config.h         # System constants, pin assignments & FreeRTOS settings
│   ├── display.h        # 20x4 I2C LCD driver interface
│   ├── relays.h         # Thread-safe relay driver interface
│   ├── web_server.h     # Web server & WebSocket controller interface
│   └── webpage.h        # Web UI dashboard stored in PROGMEM
├── src/                 # Implementation source files
│   ├── main.cpp         # Main entry point & FreeRTOS task manager
│   ├── config.cpp       # Wi-Fi credentials & channel definitions
│   ├── display.cpp      # Thread-safe 20x4 LCD telemetry driver
│   ├── relays.cpp       # Thread-safe relay implementation with FreeRTOS mutex
│   └── web_server.cpp   # REST API & WebSocket handlers
└── README.md
```

---

## ⚙️ FreeRTOS Architecture

| Task | Core | Priority | Stack Depth | Description |
| :--- | :---: | :---: | :---: | :--- |
| `WebServerTask` | **Core 0** | 1 | 4096 bytes | Dedicated HTTP server & WebSocket event handling; delivers sub-15ms low-latency commands without blocking hardware operations. |
| `SystemMonitorTask` | **Core 1** | 1 | 2048 bytes | Diagnostics & watchdog; monitors Wi-Fi connection, auto-reconnects on drop, and logs heap statistics. |
| `DisplayTask` | **Core 1** | 1 | 3072 bytes | Periodic telemetry update to the 20x4 I2C LCD display (WiFi, IP, live channel states, RSSI, and Uptime). |
| `Relay Mutex` | — | — | — | `SemaphoreHandle_t` protects relay states and GPIO pins against race conditions across concurrent tasks. |
| `LCD Mutex` | — | — | — | `SemaphoreHandle_t` prevents I2C bus collision across tasks. |

---

## 📟 20x4 I2C LCD Display Mapping

| Signal | ESP32 GPIO | Description |
| :---: | :---: | :--- |
| **SDA** | 21 | I2C Data line |
| **SCL** | 22 | I2C Clock line |
| **I2C Address** | `0x27` / `0x3F` | Auto-detected on startup |

- **Line 0:** System Name & Link Status (`[LIVE]` or `[WAIT]`)
- **Line 1:** IP Address (`IP: 192.168.x.x`) or Wi-Fi status
- **Line 2:** Active Relays count (`Active: 3/10 [RUN]`) or transient banner (`CH1 ON: Water Inlet`)
- **Line 3:** System Uptime & Wi-Fi signal (`Up: 00:04:12 -65dBm`)

---

## 🔌 Valve & Pump Channel Mapping

| Channel | GPIO | Logic Level | Dialyzer Cleaning Function |
| :---: | :---: | :---: | :--- |
| **1** | 13 | Active LOW | Water Inlet Valve |
| **2** | 16 | Active LOW | Disinfectant Valve |
| **3** | 17 | Active LOW | Blood Port Flush Valve |
| **4** | 5  | Active LOW | Dialysate Port Flush Valve |
| **5** | 18 | Active LOW | Reverse Flow Valve |
| **6** | 19 | Active LOW | Drain Release Valve |
| **7** | 23 | Active LOW | Main Booster Pump |
| **8** | 25 | Active LOW | Chemical Metering Pump |
| **9** | 27 | **Active HIGH** | Air Purge / Vent Valve |
| **10** | 26 | **Active HIGH** | Waste Discharge Valve |

---

## 🌐 Web API Endpoints

- `GET /` — Serves the responsive web dashboard from flash memory (`PROGMEM`).
- `GET /relay?ch=<0-9>&state=<0|1>` — Updates individual valve or pump state.
- `GET /all?state=<0|1>` — Activates or deactivates all channels at once.
- `GET /status` — Returns a JSON array of all 10 channel states.
- `GET /info` — Returns system diagnostics (uptime, free heap, firmware version).

---

## 🚀 Building & Flashing (PlatformIO)

```bash
# Build firmware
pio run

# Upload to ESP32
pio run --target upload

# Open Serial Monitor (115200 baud)
pio device monitor
```
