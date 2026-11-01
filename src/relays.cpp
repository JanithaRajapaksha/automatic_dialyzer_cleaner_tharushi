#include "relays.h"
#include "display.h"

// Internal relay state tracking
static bool s_relayState[NUM_RELAYS] = {false};

// FreeRTOS Mutex to ensure thread-safe relay operations across tasks
static SemaphoreHandle_t s_relayMutex = NULL;

void initRelays() {
  // Create FreeRTOS Mutex for relay operations
  if (s_relayMutex == NULL) {
    s_relayMutex = xSemaphoreCreateMutex();
  }

  // Set each relay pin to OUTPUT and immediately apply the safe OFF state.
  // In ESP32 Core 3.x, pinMode() must be called BEFORE digitalWrite()
  // for the GPIO driver to register and output the desired level.
  for (int i = 0; i < NUM_RELAYS; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    digitalWrite(RELAY_PINS[i], getRelayPinLevel(i, false));
    s_relayState[i] = false;
  }
}

bool setRelay(int ch, bool state) {
  if (ch < 0 || ch >= NUM_RELAYS) {
    return false;
  }

  if (s_relayMutex != NULL) {
    if (xSemaphoreTake(s_relayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      s_relayState[ch] = state;
      digitalWrite(RELAY_PINS[ch], getRelayPinLevel(ch, state));
      xSemaphoreGive(s_relayMutex);
      displayNotifyRelayChange(ch, state);
      return true;
    }
  } else {
    s_relayState[ch] = state;
    digitalWrite(RELAY_PINS[ch], getRelayPinLevel(ch, state));
    displayNotifyRelayChange(ch, state);
    return true;
  }

  return false;
}

void setAllRelays(bool state) {
  if (s_relayMutex != NULL) {
    if (xSemaphoreTake(s_relayMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
      for (int i = 0; i < NUM_RELAYS; i++) {
        s_relayState[i] = state;
        digitalWrite(RELAY_PINS[i], getRelayPinLevel(i, state));
      }
      xSemaphoreGive(s_relayMutex);
      displayNotifyRelayChange(-1, state);
    }
  } else {
    for (int i = 0; i < NUM_RELAYS; i++) {
      s_relayState[i] = state;
      digitalWrite(RELAY_PINS[i], getRelayPinLevel(i, state));
    }
    displayNotifyRelayChange(-1, state);
  }
}

bool getRelayState(int ch) {
  if (ch < 0 || ch >= NUM_RELAYS) {
    return false;
  }

  bool state = false;
  if (s_relayMutex != NULL) {
    if (xSemaphoreTake(s_relayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      state = s_relayState[ch];
      xSemaphoreGive(s_relayMutex);
    }
  } else {
    state = s_relayState[ch];
  }

  return state;
}

void getAllRelayStates(bool* statesOut, int maxCount) {
  if (statesOut == NULL || maxCount <= 0) {
    return;
  }

  int limit = (maxCount < NUM_RELAYS) ? maxCount : NUM_RELAYS;

  if (s_relayMutex != NULL) {
    if (xSemaphoreTake(s_relayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      for (int i = 0; i < limit; i++) {
        statesOut[i] = s_relayState[i];
      }
      xSemaphoreGive(s_relayMutex);
    }
  } else {
    for (int i = 0; i < limit; i++) {
      statesOut[i] = s_relayState[i];
    }
  }
}

const char* getRelayName(int ch) {
  if (ch < 0 || ch >= NUM_RELAYS) {
    return "Unknown Valve";
  }
  return RELAY_NAMES[ch];
}
