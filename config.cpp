#include "config.h"

// Wi-Fi credentials
const char* WIFI_SSID     = "SLT_FIBRE";
const char* WIFI_PASSWORD = "47/2.Fbr";

// Suitable GPIOs for ESP32-WROOM-32
const int RELAY_PINS[NUM_RELAYS] = {
  13, 16, 17, 5, 18,
  19, 23, 25, 27, 26
};

// Functional descriptions for Automatic Dialyzer Cleaner channels
const char* RELAY_NAMES[NUM_RELAYS] = {
  "Water Inlet Valve",
  "Disinfectant Valve",
  "Blood Port Flush Valve",
  "Dialysate Port Flush Valve",
  "Reverse Flow Valve",
  "Drain Release Valve",
  "Main Booster Pump",
  "Chemical Metering Pump",
  "Air Purge / Vent Valve",
  "Waste Discharge Valve"
};

// Active trigger levels:
// Channels 1 to 8:  Active LOW  (LOW turns relay ON, HIGH turns relay OFF)
// Channels 9 to 10: Active HIGH (HIGH turns relay ON, LOW turns relay OFF)
const uint8_t RELAY_ACTIVE_LEVELS[NUM_RELAYS] = {
  LOW,  // Relay 1  (Active LOW)
  LOW,  // Relay 2  (Active LOW)
  LOW,  // Relay 3  (Active LOW)
  LOW,  // Relay 4  (Active LOW)
  LOW,  // Relay 5  (Active LOW)
  LOW,  // Relay 6  (Active LOW)
  LOW,  // Relay 7  (Active LOW)
  LOW,  // Relay 8  (Active LOW)
  HIGH, // Relay 9  (Active HIGH)
  HIGH  // Relay 10 (Active HIGH)
};

