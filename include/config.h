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
// Web Server Configuration
// ==========================================
#define WEB_SERVER_PORT 80

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
// FreeRTOS Configuration
// ==========================================
#define WEBSERVER_TASK_STACK_SIZE    4096
#define WEBSERVER_TASK_PRIORITY      1
#define WEBSERVER_TASK_CORE          0  // Run networking on Core 0

#define MONITOR_TASK_STACK_SIZE      2048
#define MONITOR_TASK_PRIORITY        1
#define MONITOR_TASK_CORE            1  // Run system monitor on Core 1

#endif // CONFIG_H

