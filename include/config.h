#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ==========================================
// Project Information
// ==========================================
#define SYSTEM_NAME "Automatic Dialyzer Cleaner"
#define SYSTEM_VERSION "1.0.0"

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
// Relay Configuration (ESP32-WROOM-32)
// ==========================================
#define NUM_RELAYS 10

// GPIO pins assigned to relays
extern const int RELAY_PINS[NUM_RELAYS];

// Channel names for Automatic Dialyzer Cleaner
extern const char* RELAY_NAMES[NUM_RELAYS];

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
#define WEBSERVER_TASK_STACK_SIZE    4096
#define WEBSERVER_TASK_PRIORITY      1
#define WEBSERVER_TASK_CORE          0  // Run networking on Core 0

#define MONITOR_TASK_STACK_SIZE      2048
#define MONITOR_TASK_PRIORITY        1
#define MONITOR_TASK_CORE            1  // Run system monitor on Core 1

#define DISPLAY_TASK_STACK_SIZE      3072
#define DISPLAY_TASK_PRIORITY        1
#define DISPLAY_TASK_CORE            1  // Run display updates on Core 1

#endif // CONFIG_H

