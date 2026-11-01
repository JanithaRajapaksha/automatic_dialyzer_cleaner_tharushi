#include "config.h"

// Wi-Fi credentials
const char* WIFI_SSID     = "SLT_FIBRE";
const char* WIFI_PASSWORD = "47/2.Fbr";

// Suitable GPIOs for ESP32-WROOM-32
const int RELAY_PINS[NUM_RELAYS] = {
  13, 16, 17, 5, 18,
  19, 23, 25, 27, 26
};

// Functional descriptions for Automatic Dialyzer Reprocessor actuators
const char* RELAY_NAMES[NUM_RELAYS] = {
  "Pump 1: Primary RO Water Pump",
  "Pump 2: Chemical / H2O2 Flush Pump",
  "Pump 3: PAA / Renalin Sterilant Pump",
  "Valve S1: RO / Fluid Input Control",
  "Valve S2: Intermediate Port / Chemical Path",
  "Valve S3: Recirculation Control",
  "Valve S4: Dialyzer Blood / Dialyzate Side Port",
  "Valve S5: Pressure Test Isolation Valve",
  "Valve S6: Exhaust / Bleed / Drain Valve",
  "Air Pump: Pneumatic Pressure Source"
};

// Short actuator identifiers
const char* ACTUATOR_CODES[NUM_RELAYS] = {
  "P1", "P2", "P3", "S1", "S2", "S3", "S4", "S5", "S6", "AIR"
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

