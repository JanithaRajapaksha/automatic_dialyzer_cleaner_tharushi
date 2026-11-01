# Automatic Dialyzer Reprocessor - Embedded Control System

Modular ESP32 firmware running FreeRTOS for an Automated Dialyzer Reprocessing unit, built for **PlatformIO**. Features an asynchronous finite state machine (FSM), directional valve-before-pump interlocking, sub-microsecond hardware emergency shutdown, persistent real-time WebSockets, 20x4 I2C LCD telemetry, and a responsive web dashboard.

---

## 📁 PlatformIO Project Structure

```
automatic_dialyzer_cleaner_tharushi/
├── platformio.ini       # PlatformIO configuration (esp32dev, libraries, upload speed)
├── include/             # Header files
│   ├── config.h         # System constants, pin definitions, timing & FreeRTOS settings
│   ├── display.h        # 20x4 I2C LCD driver interface
│   ├── pressure_sensor.h# HX710B 24-bit ADC air pressure sensor driver & calibration
│   ├── relays.h         # Thread-safe relay driver & immediate hardware E-Stop
│   ├── reprocessor.h    # Asynchronous FSM sequencer & process stage definitions
│   ├── web_server.h     # HTTP REST server & real-time WebSocket controller
│   └── webpage.h        # Web UI dashboard stored in PROGMEM (cache-busting headers)
├── src/                 # Implementation source files
│   ├── main.cpp         # Main entry point & FreeRTOS task launcher
│   ├── config.cpp       # Actuator definitions, pin maps & Wi-Fi credentials
│   ├── display.cpp      # Live 20x4 LCD telemetry driver (Auto, Manual, Idle)
│   ├── pressure_sensor.cpp # HX710B bit-bang reader, tare, conversions (mmHg/kPa/psi)
│   ├── relays.cpp       # Mutex-protected GPIO control & direct register E-Stop
│   ├── reprocessor.cpp  # Reprocessor sequential FSM engine & interlock timers
│   └── web_server.cpp   # REST API & WebSocket command processor
└── README.md
```

---

## ⚙️ FreeRTOS Multi-Core Architecture

| Task | Core | Priority | Stack Depth | Description |
| :--- | :---: | :---: | :---: | :--- |
| `WebServerTask` | **Core 0** | 1 | 8192 bytes | High-speed HTTP server (port 80) and WebSocket server (port 81); handles bidirectional streaming telemetry and instant command dispatch. |
| `ReprocessorTask` | **Core 1** | 2 | 4096 bytes | Dedicated asynchronous finite state machine (20 Hz); handles process stages, precision sub-step timers, and valve-pump interlocks. |
| `DisplayTask` | **Core 1** | 1 | 4096 bytes | Live telemetry update to the 20x4 I2C LCD display (active stage, timer, countdown, active actuators, WiFi, IP, and RSSI). |
| `SystemMonitorTask` | **Core 1** | 1 | 4096 bytes | Diagnostics & watchdog; reads HX710B pressure sensor (10 Hz), monitors Wi-Fi connection, auto-reconnects on drop, and logs heap statistics. |
| `Sensor Mutex` | — | — | — | `SemaphoreHandle_t` prevents race conditions while reading HX710B 24-bit pressure data across tasks. |
| `Relay Mutex` | — | — | — | `SemaphoreHandle_t` protects relay states and GPIO pins against race conditions across tasks. |
| `LCD Mutex` | — | — | — | `SemaphoreHandle_t` prevents I2C bus collision across tasks. |

---

## 🔌 Actuator to Relay Hardware Mapping

The controller drives 10 independent SPDT/SPST relays assigned to the fluidic and pneumatic components:

| Relay | Code | Actuator Assignment | Description / Subsystem | GPIO | Logic Level |
| :---: | :---: | :--- | :--- | :---: | :---: |
| **R1** | `P1` | **Pump 1** | Primary RO (Reverse Osmosis) Water Pump | 13 | Active LOW |
| **R2** | `P2` | **Pump 2** | Chemical / $\text{H}_2\text{O}_2$ Flushing Pump | 16 | Active LOW |
| **R3** | `P3` | **Pump 3** | PAA / Renalin Sterilant Pump | 17 | Active LOW |
| **R4** | `S1` | **Solenoid Valve 1** | RO / Fluid Input Control | 5 | Active LOW |
| **R5** | `S2` | **Solenoid Valve 2** | Intermediate Port / Chemical Path | 18 | Active LOW |
| **R6** | `S3` | **Solenoid Valve 3** | Intermediate / Recirculation Control | 19 | Active LOW |
| **R7** | `S4` | **Solenoid Valve 4** | Dialyzer Blood / Dialyzate Side Port | 23 | Active LOW |
| **R8** | `S5` | **Solenoid Valve 5** | Pressure Test Isolation Valve | 25 | Active LOW |
| **R9** | `S6` | **Solenoid Valve 6** | Exhaust / Bleed / Drain Valve | 27 | **Active HIGH** |
| **R10**| `AIR`| **Air Pump** | Pneumatic Pressure Generation Source | 26 | **Active HIGH** |

---

## 🌬️ HX710B Air Pressure Sensor Mapping

The pneumatic leak test and chamber pressure are monitored via a 24-bit differential ADC pressure sensor (HX710B / MPS20N0040D-D, 0–40 kPa):

| Signal | ESP32 GPIO | Description | Direction |
| :---: | :---: | :--- | :---: |
| **DOUT** | 32 | Serial Data Output (goes LOW when 24-bit data is ready) | Input |
| **SCLK** | 14 | Serial Clock Input (24 bits + 1 channel select pulse) | Output |

- **Sample Rate**: 10 Hz / 40 Hz
- **Units Provided**: mmHg, kPa, psi, and raw 24-bit ADC counts
- **Auto-Tare**: Establishes zero-offset reference at atmospheric pressure on boot or on-demand (`/pressure/tare` or WebSocket `TARE`).

---

## 🔄 Global State Matrix & Sequential Stages

All relays default to 0 (OFF / De-energized) when entering `STATE_IDLE` or during an emergency shutdown.

| State ID | Process Stage Name | Active Relays (ON) | Inactive Relays (OFF) | Default Time |
| :---: | :--- | :--- | :--- | :---: |
| **0** | **System Idle** | None | R1 – R10 | — |
| **1** | **RO Wash (Both Sides)** | R1, R4, R7, R9 | R2, R3, R5, R6, R8, R10 | 40 s |
| **2** | **$\text{H}_2\text{O}_2$ Chemical Wash** | R2, R5, R7, R9 | R1, R3, R4, R6, R8, R10 | 50 s |
| **3** | **Secondary RO Rinse** | R1, R4, R7, R9 | R2, R3, R5, R6, R8, R10 | 40 s |
| **4A** | **Pressure Test: Pressurization** | R8, R10 | R1 – R7, R9 | 10 s |
| **4B** | **Pressure Test: Stabilization / Decay** | None (Air Lock) | R1 – R10 | 15 s |
| **4C** | **Pressure Test: Vent / Depressurize** | R9 ($S_6$ Vent) | R1 – R8, R10 | 5 s |
| **5** | **RO Water Fill (Blood Side / FBV)** | R1, R4, R9 | R2, R3, R5, R6, R7, R8, R10 | 20 s |
| **6** | **FBV Test Measurement** | R6 ($S_3$) | R1 – R5, R7 – R10 | 15 s |
| **7** | **PAA / Renalin Sanitization & Storage** | R3, R6, R7 | R1, R2, R4, R5, R8, R9, R10 | 25 s |

---

## 🛡️ Safety Interlocking & Emergency E-Stop

1. **Valve-Before-Pump Interlock ($200\text{ ms}$)**:
   - Solenoid valves ($S_1\text{--}S_6$) are energized $100\text{--}250\text{ ms}$ before turning on positive displacement pumps ($P_1, P_2, P_3$) or the compressor ($R_{10}$) to prevent line deadheading and pressure spikes.
2. **Pump-Before-Valve Interlock ($150\text{ ms}$)**:
   - Pumps are powered down $100\text{--}200\text{ ms}$ before closing solenoid valves to prevent fluid hammer.
3. **Hardware Emergency Stop (E-Stop)**:
   - Invoking `emergencyStop()` immediately clears the physical GPIO output pins without waiting for mutex locks ($< 1\ \mu\text{s}$ response), de-energizes all 10 relays, and transitions the state machine to `STAGE_ABORTED`.

---

## 📟 20x4 I2C LCD Display

The firmware auto-probes the PCF8574 I2C expander at addresses `0x27` and `0x3F` on SDA (GPIO 21) and SCL (GPIO 22).

### Active Cycle Display (Auto or Single Stage)
```
+--------------------+
|REPROCESSOR   AUTO24%| Line 0: Mode & Progress %
|1/7 RO WASH (BOTH)  | Line 1: Active Process Stage
|PUMP RUN    28s/40s | Line 2: Sub-step & Countdown
|ACT: P1, S1, S4, S6 | Line 3: Active Actuators
+--------------------+
```

### Standby Display (Idle)
```
+--------------------+
|DIALYZER REP  [LIVE]| Line 0: System Name & Status
|IP: 192.168.1.15    | Line 1: Network IP Address
|Status: Ready (Idle)| Line 2: Ready Banner
|Up:00:14:32  -58dBm | Line 3: Uptime & Wi-Fi RSSI
+--------------------+
```

---

## 🌐 Web API & Real-Time WebSocket Protocol

### HTTP REST Endpoints

| Endpoint | Method | Description |
| :--- | :---: | :--- |
| `GET /` | HTTP | Serves the web dashboard (includes `Cache-Control: no-cache` headers). |
| `GET /status` | HTTP | Returns JSON relay bitmask: `{"type":"status","states":[...],"0":...}`. |
| `GET /info` | HTTP | Returns system diagnostic telemetry (uptime, free heap, firmware version). |
| `GET /relay?ch=<0-9>&state=<0\|1>` | HTTP | Manual toggle of individual actuator relay. |
| `GET /all?state=<0\|1>` | HTTP | Bulk override of all 10 relays. |
| `GET /reprocessor/start` | HTTP | Starts the automated 7-stage sequence (Stages 1 through 7). |
| `GET /reprocessor/stage?id=<1-7>` | HTTP | Executes an individual stage with full interlock sequencing. |
| `GET /reprocessor/pause` | HTTP | Pauses the active cycle (safely shuts down pumps). |
| `GET /reprocessor/resume` | HTTP | Resumes the paused cycle. |
| `GET /reprocessor/abort` | HTTP | Aborts active cycle and returns relays to safe OFF. |
| `GET /reprocessor/estop` | HTTP | Immediate hardware emergency stop. |
| `GET /reprocessor/status` | HTTP | Returns full FSM telemetry JSON. |
| `GET /pressure` | HTTP | Returns live HX710B sensor reading (raw counts, tare, mmHg, kPa, psi). |
| `GET /pressure/tare` | HTTP | Samples atmospheric zero baseline and resets gauge reading to 0. |
| `GET /pressure/scale?val=<float>` | HTTP | Sets custom calibration factor (counts per kPa). |

### WebSocket Commands (Port 81)

- `START` — Starts automated reprocessing cycle.
- `STAGE:<1-7>` — Executes single stage by ID.
- `PAUSE` / `RESUME` — Pauses or resumes running cycle.
- `ABORT` / `STOP` — Aborts cycle and resets to idle.
- `ESTOP` — Triggers immediate hardware emergency stop.
- `TARE` — Tares the HX710B air pressure sensor to 0 mmHg baseline.
- `PRESSURE` — Requests immediate pressure reading frame.
- `SET:<ch>:<0|1>` — Fast compact relay toggle (e.g. `SET:0:1` turns on Pump 1).
- `ALL:<0|1>` — Fast bulk relay toggle (e.g. `ALL:0` turns off all relays).
- `STATUS` — Requests immediate relay bitmask and reprocessor telemetry push.

---

## 🚀 Building & Flashing (PlatformIO)

```bash
# Build firmware
pio run

# Upload to ESP32 via USB
pio run --target upload

# Open Serial Monitor (115200 baud)
pio device monitor
```
