#include "display.h"
#include "relays.h"
#include "reprocessor.h"
#include "pressure_sensor.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>

static LiquidCrystal_I2C* lcd = nullptr;
static SemaphoreHandle_t lcdMutex = NULL;
static uint8_t activeLcdAddr = LCD_I2C_ADDR_PRIMARY;
static bool lcdAvailable = false;

// Differential row cache: only write to LCD over I2C when text changes
static char s_lastLcdBuffer[LCD_ROWS][LCD_COLS + 1] = { "", "", "", "" };

// Custom 5-level vertical column characters for high-resolution analog bar graph (1/5th to 5/5th)
static const uint8_t BAR_1[8] = { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10 };
static const uint8_t BAR_2[8] = { 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18 };
static const uint8_t BAR_3[8] = { 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C };
static const uint8_t BAR_4[8] = { 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E };
static const uint8_t BAR_5[8] = { 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F };

// Transient action banner (shows last toggled relay for 1.8 seconds)
static int lastActionChannel = -2;
static bool lastActionState = false;
static unsigned long lastActionTime = 0;

static void printLine(uint8_t row, const char* text) {
  if (!lcdAvailable || !lcd || row >= LCD_ROWS) return;
  char buf[LCD_COLS + 1];
  snprintf(buf, sizeof(buf), "%-20.20s", text);

  // Only transmit over I2C if this line actually changed
  if (memcmp(s_lastLcdBuffer[row], buf, LCD_COLS) != 0) {
    memcpy(s_lastLcdBuffer[row], buf, LCD_COLS);
    s_lastLcdBuffer[row][LCD_COLS] = '\0';
    lcd->setCursor(0, row);
    for (int i = 0; i < LCD_COLS; i++) {
      lcd->write((uint8_t)buf[i]);
    }
  }
}

// Generate an 18-character dynamic analog bar gauge enclosed in '[' and ']'
static void makePressureBar(char* outBuf, float mmhg, float maxMmhg = 250.0f) {
  const int barChars = 18; // 18 characters enclosed by '[' and ']' = 20 chars
  if (mmhg < 0.0f) mmhg = 0.0f;
  if (mmhg > maxMmhg) mmhg = maxMmhg;

  int totalPixels = (int)((mmhg / maxMmhg) * (barChars * 5) + 0.5f);
  if (totalPixels > barChars * 5) totalPixels = barChars * 5;

  outBuf[0] = '[';
  int fullChars = totalPixels / 5;
  int remPixels = totalPixels % 5;

  int idx = 1;
  for (int i = 0; i < fullChars && idx <= barChars; i++) {
    outBuf[idx++] = (char)5; // 5-pixel solid block
  }
  if (remPixels > 0 && idx <= barChars) {
    outBuf[idx++] = (char)remPixels; // sub-pixel partial column (1..4)
  }
  while (idx <= barChars) {
    outBuf[idx++] = ' ';
  }
  outBuf[idx++] = ']';
  outBuf[idx] = '\0';
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

    // Register 5-level bar graph custom characters in CGRAM
    lcd->createChar(1, (uint8_t*)BAR_1);
    lcd->createChar(2, (uint8_t*)BAR_2);
    lcd->createChar(3, (uint8_t*)BAR_3);
    lcd->createChar(4, (uint8_t*)BAR_4);
    lcd->createChar(5, (uint8_t*)BAR_5);

    lcd->clear();
    for (int r = 0; r < LCD_ROWS; r++) {
      memset(s_lastLcdBuffer[r], 0, sizeof(s_lastLcdBuffer[r]));
    }
    Serial.printf("[LCD] 20x4 I2C LCD detected and initialized at 0x%02X with custom graphics\n", activeLcdAddr);
  } else {
    Serial.println("[LCD] Warning: No 20x4 I2C LCD detected at 0x27 or 0x3F. Continuing headless.");
  }
}

void displayBootScreen() {
  if (!lcdAvailable || lcdMutex == NULL) return;

  if (xSemaphoreTake(lcdMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    printLine(0, "====================");
    printLine(1, "DIALYZER REPROCESSOR");
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
  if (!lcdAvailable || !lcd || lcdMutex == NULL) return;

  if (xSemaphoreTake(lcdMutex, pdMS_TO_TICKS(40)) != pdTRUE) {
    return;
  }

  char line[40];
  ReprocessorStatus rep;
  getReprocessorStatus(&rep);

  PressureData p;
  getPressureData(&p);

  // ----------------------------------------------------
  // Scenario A: Reprocessor Cycle Active
  // ----------------------------------------------------
  if (rep.isRunning || rep.isPaused || rep.isAborted) {
    // Line 0: Header & Cycle Status
    if (rep.isAborted) {
      printLine(0, "REPROCESSOR  [ABORT]");
    } else if (rep.isPaused) {
      printLine(0, "REPROCESSOR  [PAUSE]");
    } else if (rep.isAutoCycle) {
      snprintf(line, sizeof(line), "AUTO %2d%% STG %-10.10s", rep.progressPercent, rep.stageName);
      printLine(0, line);
    } else {
      snprintf(line, sizeof(line), "STAGE %d: %-10.10s", rep.stageNumber, rep.stageName);
      printLine(0, line);
    }

    // Line 1: Live Pressure Readout (10 Hz)
    if (p.isConnected) {
      snprintf(line, sizeof(line), "P:%5.1fmmHg %4.1fkPa ", p.pressureMmhg, p.pressureKPa);
    } else {
      snprintf(line, sizeof(line), "Air P: SENSOR ERROR ");
    }
    printLine(1, line);

    // Line 2: Real-time Analog Bar Graph (0 to 250 mmHg)
    char barBuf[25];
    makePressureBar(barBuf, p.isConnected ? p.pressureMmhg : 0.0f, 250.0f);
    printLine(2, barBuf);

    // Line 3: Sub-step & Countdown / Active Actuators
    if (rep.stageTotalSec > 0) {
      snprintf(line, sizeof(line), "%-9.9s %2lus/%2lus",
               rep.subStepName,
               rep.stageElapsedSec,
               rep.stageTotalSec);
    } else {
      snprintf(line, sizeof(line), "ACT: %-15.15s", rep.activeActuators);
    }
    printLine(3, line);
  }
  // ----------------------------------------------------
  // Scenario B: System Idle / Manual Control
  // ----------------------------------------------------
  else {
    bool isConnected = (WiFi.status() == WL_CONNECTED);

    // Line 0: Status or Transient Manual Action Banner
    bool showRecentAction = (lastActionChannel >= -1) && (millis() - lastActionTime < 1800);
    if (showRecentAction) {
      if (lastActionChannel == -1) {
        snprintf(line, sizeof(line), "ALL: %s", lastActionState ? "ACTIVE (ON)" : "SAFE (OFF)");
      } else {
        snprintf(line, sizeof(line), "MAN: %s (R%d) %s",
                 ACTUATOR_CODES[lastActionChannel],
                 lastActionChannel + 1,
                 lastActionState ? "ON" : "OFF");
      }
    } else {
      if (isConnected) {
        snprintf(line, sizeof(line), "DIALYZER REP  [LIVE]");
      } else {
        snprintf(line, sizeof(line), "DIALYZER REP  [WAIT]");
      }
    }
    printLine(0, line);

    // Line 1: Real-time Pressure Readout (10 Hz)
    if (p.isConnected) {
      snprintf(line, sizeof(line), "P:%5.1fmmHg %4.1fkPa ", p.pressureMmhg, p.pressureKPa);
    } else {
      snprintf(line, sizeof(line), "Air P: Connecting...");
    }
    printLine(1, line);

    // Line 2: Real-time Analog Bar Graph (0 to 250 mmHg)
    char barBuf[25];
    makePressureBar(barBuf, p.isConnected ? p.pressureMmhg : 0.0f, 250.0f);
    printLine(2, barBuf);

    // Line 3: IP Address & Uptime
    unsigned long sec = millis() / 1000;
    unsigned long mins = (sec % 3600) / 60;
    unsigned long secs = sec % 60;
    if (isConnected) {
      snprintf(line, sizeof(line), "%-11.11s Up:%02lu:%02lu", WiFi.localIP().toString().c_str(), mins, secs);
    } else {
      snprintf(line, sizeof(line), "No WiFi  Up:%02lu:%02lu", mins, secs);
    }
    printLine(3, line);
  }

  xSemaphoreGive(lcdMutex);
}

void displayTask(void* pvParameters) {
  Serial.println("[FreeRTOS] Display Task started on Core " + String(xPortGetCoreID()));

  for (;;) {
    updateDisplay();
    // Refresh display at 10 Hz (every 100ms) with zero flicker via differential row caching
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
