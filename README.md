# Automatic Dialyzer Cleaner - Embedded Control System

Modular ESP32 firmware running FreeRTOS for a 10-channel Automated Dialyzer Cleaner controller.

---

## 📁 Project Structure

```
tharushi_fyp_1/
├── tharushi_fyp_1.ino    # Main entry point & FreeRTOS task manager
├── config.h              # Global configuration constants, pin maps & task settings
├── config.cpp            # Wi-Fi credentials & channel label definitions
├── relays.h              # Thread-safe relay control interface
├── relays.cpp            # Relay driver with FreeRTOS mutex synchronization
├── webpage.h             # Responsive web UI dashboard (stored in PROGMEM)
├── web_server.h          # Web server controller interface
└── web_server.cpp        # HTTP endpoint handlers & JSON API
```

---

## ⚙️ FreeRTOS Architecture

| Task | Core | Priority | Stack Depth | Description |
| :--- | :---: | :---: | :---: | :--- |
| `WebServerTask` | **Core 0** | 1 | 4096 bytes | Dedicated HTTP server request handling; processes web dashboard interactions without blocking hardware operations. |
| `SystemMonitorTask` | **Core 1** | 1 | 2048 bytes | Diagnostics & watchdog; monitors Wi-Fi connection, auto-reconnects on drop, and logs heap statistics. |
| `Relay Mutex` | — | — | — | `SemaphoreHandle_t` protects relay states and GPIO pins against race conditions across concurrent tasks. |

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

