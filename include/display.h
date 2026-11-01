#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include "config.h"

// Initialize I2C bus and the 20x4 LCD
void initDisplay();

// Show the boot splash screen
void displayBootScreen();

// Update display with live status (WiFi, IP, Relays, Uptime, RSSI)
void updateDisplay();

// Notify display immediately of a relay channel state change
// Pass ch = -1 for bulk "ALL" commands
void displayNotifyRelayChange(int ch, bool state);

// FreeRTOS Task function for periodic LCD refreshing
void displayTask(void* pvParameters);

#endif // DISPLAY_H

