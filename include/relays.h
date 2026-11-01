#ifndef RELAYS_H
#define RELAYS_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "config.h"

// Initialize GPIO pins, set default OFF state, and create RTOS mutex
void initRelays();

// Thread-safe update of a single relay
bool setRelay(int ch, bool state);

// Thread-safe update of all relays
void setAllRelays(bool state);

// Immediate hardware emergency shutdown: sets all 10 relays to OFF immediately
void emergencyStopRelays();

// Thread-safe read of a single relay state
bool getRelayState(int ch);

// Thread-safe read of all relay states into a buffer
void getAllRelayStates(bool* statesOut, int maxCount);

// Get functional name of a relay channel
const char* getRelayName(int ch);

#endif // RELAYS_H

