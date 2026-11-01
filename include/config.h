#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ==========================================
// Project Information
// ==========================================
#define SYSTEM_NAME "Automatic Dialyzer Reprocessor"
#define SYSTEM_VERSION "1.1.0"

// ==========================================
// Wi-Fi Credentials
// ==========================================
extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;

// ==========================================
// Web Server & WebSocket Configuration
// ==========================================
#define WEB_SERVER_PORT 80
#define WEBSOCKET_PORT 81

// ==========================================
// Actuator to Relay Hardware Mapping
// ==========================================
#define NUM_RELAYS 10

// Named Relay Index Definitions
#define RELAY_PUMP1    0  // R1: Pump 1 - Primary RO Water Pump
#define RELAY_PUMP2    1  // R2: Pump 2 - Chemical / H2O2 Flushing Pump
#define RELAY_PUMP3    2  // R3: Pump 3 - PAA / Renalin Sterilant Pump
#define RELAY_VALVE1   3  // R4: Solenoid Valve 1 (S1) - RO / Fluid Input Control
#define RELAY_VALVE2   4  // R5: Solenoid Valve 2 (S2) - Intermediate Port / Chemical Path
#define RELAY_VALVE3   5  // R6: Solenoid Valve 3 (S3) - Intermediate / Recirculation Control
#define RELAY_VALVE4   6  // R7: Solenoid Valve 4 (S4) - Dialyzer Blood / Dialyzate Side Port
#define RELAY_VALVE5   7  // R8: Solenoid Valve 5 (S5) - Pressure Test Isolation Valve
#define RELAY_VALVE6   8  // R9: Solenoid Valve 6 (S6) - Exhaust / Bleed / Drain Valve
#define RELAY_AIRPUMP  9  // R10: Air Pump - Pneumatic Pressure Generation Source

// ==========================================
// HX710B Air Pressure Sensor Configuration
// ==========================================
#define HX710_DOUT_PIN 32  // Data Out pin
#define HX710_SCLK_PIN 14  // Serial Clock pin

// GPIO pins assigned to relays
extern const int RELAY_PINS[NUM_RELAYS];

// Channel names for Automatic Dialyzer Reprocessor
extern const char* RELAY_NAMES[NUM_RELAYS];

// Actuator short codes (P1, P2, P3, S1, S2, S3, S4, S5, S6, AIR)
extern const char* ACTUATOR_CODES[NUM_RELAYS];

// Relay logic levels (per-channel trigger polarity)
// Relays 1 to 8 (indices 0 to 7): Active LOW (LOW = ON, HIGH = OFF)
// Relays 9 and 10 (indices 8 to 9): Active HIGH (HIGH = ON, LOW = OFF)
extern const uint8_t RELAY_ACTIVE_LEVELS[NUM_RELAYS];

// Helper to determine GPIO level for a given relay state (true = ON, false = OFF)
inline uint8_t getRelayPinLevel(int ch, bool state) {
  if (ch < 0 || ch >= NUM_RELAYS) return LOW;
  uint8_t onLevel = RELAY_ACTIVE_LEVELS[ch];
  uint8_t offLevel = (onLevel == HIGH) ? LOW : HIGH;
  return state ? onLevel : offLevel;
}

// ==========================================
// 20x4 I2C LCD Configuration (HD44780 / PCF8574)
// ==========================================
#define LCD_I2C_ADDR_PRIMARY   0x27   // Default PCF8574 I2C address
#define LCD_I2C_ADDR_SECONDARY 0x3F   // Alternate PCF8574A I2C address
#define LCD_COLS               20
#define LCD_ROWS               4
#define LCD_SDA_PIN            21     // Default ESP32 I2C SDA
#define LCD_SCL_PIN            22     // Default ESP32 I2C SCL

// ==========================================
// FreeRTOS Configuration
// ==========================================
#define WEBSERVER_TASK_STACK_SIZE    8192
#define WEBSERVER_TASK_PRIORITY      1
#define WEBSERVER_TASK_CORE          0  // Run networking on Core 0

#define MONITOR_TASK_STACK_SIZE      4096
#define MONITOR_TASK_PRIORITY        1
#define MONITOR_TASK_CORE            1  // Run system monitor on Core 1

#define DISPLAY_TASK_STACK_SIZE      4096
#define DISPLAY_TASK_PRIORITY        1
#define DISPLAY_TASK_CORE            1  // Run display updates on Core 1

#define REPROCESSOR_TASK_STACK_SIZE  4096
#define REPROCESSOR_TASK_PRIORITY    2  // High priority for precise timing
#define REPROCESSOR_TASK_CORE        1  // Run reprocessor FSM on Core 1

#endif // CONFIG_H

