#include "display.h"
#include "relays.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>

static LiquidCrystal_I2C* lcd = nullptr;
static SemaphoreHandle_t lcdMutex = NULL;
static uint8_t activeLcdAddr = LCD_I2C_ADDR_PRIMARY;
static bool lcdAvailable = false;

// Transient action banner (shows last toggled relay for 3 seconds)
static int lastActionChannel = -2;
static bool lastActionState = false;
static unsigned long lastActionTime = 0;

static void printLine(uint8_t row, const char* text) {
  if (!lcdAvailable || !lcd) return;
  char buf[LCD_COLS + 1];
  snprintf(buf, sizeof(buf), "%-20.20s", text);
  lcd->setCursor(0, row);
  lcd->print(buf);
}

void initDisplay() {
  if (lcdMutex == NULL) {
    lcdMutex = xSemaphoreCreateMutex();
  }

  // Initialize I2C with ESP32 default pins
  Wire.begin(LCD_SDA_PIN, LCD_SCL_PIN);
  Wire.setClock(100000); // 100 kHz standard mode for PCF8574 backpacks

  // Auto-probe I2C address (0x27 or 0x3F)
  Wire.beginTransmission(LCD_I2C_ADDR_PRIMARY);
  if (Wire.endTransmission() == 0) {
    activeLcdAddr = LCD_I2C_ADDR_PRIMARY;
    lcdAvailable = true;
  } else {
    Wire.beginTransmission(LCD_I2C_ADDR_SECONDARY);
    if (Wire.endTransmission() == 0) {
      activeLcdAddr = LCD_I2C_ADDR_SECONDARY;
      lcdAvailable = true;
    }
  }

  if (lcdAvailable) {
    lcd = new LiquidCrystal_I2C(activeLcdAddr, LCD_COLS, LCD_ROWS);
    lcd->init();
    lcd->backlight();
    lcd->clear();
    Serial.printf("[LCD] 20x4 I2C LCD detected and initialized at 0x%02X\n", activeLcdAddr);
  } else {
    Serial.println("[LCD] Warning: No 20x4 I2C LCD detected at 0x27 or 0x3F. Continuing headless.");
  }
}

void displayBootScreen() {
  if (!lcdAvailable || lcdMutex == NULL) return;

  if (xSemaphoreTake(lcdMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    printLine(0, "====================");
    printLine(1, "  DIALYZER CLEANER  ");
    printLine(2, "    v" SYSTEM_VERSION "         ");
    printLine(3, "Connecting WiFi...  ");
    xSemaphoreGive(lcdMutex);
  }
}

void displayNotifyRelayChange(int ch, bool state) {
  lastActionChannel = ch;
  lastActionState = state;
  lastActionTime = millis();
}

void updateDisplay() {
  if (!lcdAvailable || lcdMutex == NULL) return;

  if (xSemaphoreTake(lcdMutex, pdMS_TO_TICKS(50)) != pdTRUE) {
    return;
  }

  char line[40];

  // ----------------------------------------------------
  // Line 0: Header & Link Status
  // ----------------------------------------------------
  bool isConnected = (WiFi.status() == WL_CONNECTED);
  if (isConnected) {
    printLine(0, "DIALYZER CLN  [LIVE]");
  } else {
    printLine(0, "DIALYZER CLN  [WAIT]");
  }

  // ----------------------------------------------------
  // Line 1: Wi-Fi / IP Address
  // ----------------------------------------------------
  if (isConnected) {
    snprintf(line, sizeof(line), "IP: %s", WiFi.localIP().toString().c_str());
  } else {
    snprintf(line, sizeof(line), "WiFi: %s", WIFI_SSID);
  }
  printLine(1, line);

  // ----------------------------------------------------
  // Line 2: Active Relays or Recent Action Banner
  // ----------------------------------------------------
  bool showRecentAction = (lastActionChannel >= -1) && (millis() - lastActionTime < 3500);

  if (showRecentAction) {
    if (lastActionChannel == -1) {
      snprintf(line, sizeof(line), "ALL: %s", lastActionState ? "ACTIVATED (ON)" : "STOPPED (OFF)");
    } else {
      const char* name = getRelayName(lastActionChannel);
      snprintf(line, sizeof(line), "CH%d %s: %s",
               lastActionChannel + 1,
               lastActionState ? "ON" : "OFF",
               name ? name : "");
    }
  } else {
    bool states[NUM_RELAYS];
    getAllRelayStates(states, NUM_RELAYS);
    int activeCount = 0;
    for (int i = 0; i < NUM_RELAYS; i++) {
      if (states[i]) activeCount++;
    }

    if (activeCount == 0) {
      snprintf(line, sizeof(line), "Active: 0/10  [IDLE]");
    } else {
      snprintf(line, sizeof(line), "Active: %d/10  [RUN]", activeCount);
    }
  }
  printLine(2, line);

  // ----------------------------------------------------
  // Line 3: System Uptime & RSSI
  // ----------------------------------------------------
  unsigned long sec = millis() / 1000;
  unsigned long hrs = sec / 3600;
  unsigned long mins = (sec % 3600) / 60;
  unsigned long secs = sec % 60;

  int rssi = isConnected ? WiFi.RSSI() : 0;
  snprintf(line, sizeof(line), "Up:%02lu:%02lu:%02lu  %ddBm", hrs, mins, secs, rssi);
  printLine(3, line);

  xSemaphoreGive(lcdMutex);
}

void displayTask(void* pvParameters) {
  Serial.println("[FreeRTOS] Display Task started on Core " + String(xPortGetCoreID()));

  for (;;) {
    updateDisplay();
    // Refresh display every 1 second
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
