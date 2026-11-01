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
  if (!lcdAvailable || lcdMutex == NULL) return;

  if (xSemaphoreTake(lcdMutex, pdMS_TO_TICKS(50)) != pdTRUE) {
    return;
  }

  char line[40];
  ReprocessorStatus rep;
  getReprocessorStatus(&rep);

  // ----------------------------------------------------
  // Scenario A: Reprocessor Cycle Active
  // ----------------------------------------------------
  if (rep.isRunning || rep.isPaused || rep.isAborted) {
    // Line 0: Header & Cycle Type
    if (rep.isAborted) {
      printLine(0, "REPROCESSOR  [ABORT]");
    } else if (rep.isPaused) {
      printLine(0, "REPROCESSOR  [PAUSE]");
    } else if (rep.isAutoCycle) {
      snprintf(line, sizeof(line), "REPROCESSOR   AUTO%2d%%", rep.progressPercent);
      printLine(0, line);
    } else {
      snprintf(line, sizeof(line), "STAGE %d RUNNING", rep.stageNumber);
      printLine(0, line);
    }

    // Line 1: Short Stage Name
    switch (rep.stage) {
      case STAGE_RO_WASH_1:           printLine(1, "1/7 RO WASH (BOTH)  "); break;
      case STAGE_H2O2_WASH:           printLine(1, "2/7 H2O2 FLUSH      "); break;
      case STAGE_RO_WASH_2:           printLine(1, "3/7 RO RINSE 2      "); break;
      case STAGE_PRESSURE_PRESSURIZE: printLine(1, "4A: AIR PRESSURIZE  "); break;
      case STAGE_PRESSURE_HOLD:       printLine(1, "4B: LEAK TEST HOLD  "); break;
      case STAGE_PRESSURE_VENT:       printLine(1, "4C: DEPRESSURIZE    "); break;
      case STAGE_RO_FILL_BLOOD:       printLine(1, "5/7 BLOOD SIDE FILL "); break;
      case STAGE_FBV_MEASURE:         printLine(1, "6/7 FBV MEASUREMENT "); break;
      case STAGE_PAA_PRIME:           printLine(1, "7/7 PAA PRIME/STORE "); break;
      case STAGE_COMPLETE:            printLine(1, "REPROCESS COMPLETE! "); break;
      case STAGE_ABORTED:             printLine(1, "EMERGENCY STOPPED!  "); break;
      default:                        printLine(1, "CYCLE STANDBY       "); break;
    }

    // Line 2: Sub-step & Elapsed/Total Countdown
    if (rep.stageTotalSec > 0) {
      snprintf(line, sizeof(line), "%-10.10s %2lus/%2lus",
               rep.subStepName,
               rep.stageElapsedSec,
               rep.stageTotalSec);
    } else {
      snprintf(line, sizeof(line), "%-20.20s", rep.subStepName);
    }
    printLine(2, line);

    // Line 3: Active Actuators
    snprintf(line, sizeof(line), "ACT: %-15.15s", rep.activeActuators);
    printLine(3, line);
  }
  // ----------------------------------------------------
  // Scenario B: System Idle
  // ----------------------------------------------------
  else {
    bool isConnected = (WiFi.status() == WL_CONNECTED);
    if (isConnected) {
      printLine(0, "DIALYZER REP  [LIVE]");
      snprintf(line, sizeof(line), "IP: %s", WiFi.localIP().toString().c_str());
    } else {
      printLine(0, "DIALYZER REP  [WAIT]");
      snprintf(line, sizeof(line), "WiFi: %s", WIFI_SSID);
    }
    printLine(1, line);

    // Line 2: Action or Live Air Pressure Sensor
    bool showRecentAction = (lastActionChannel >= -1) && (millis() - lastActionTime < 3500);
    if (showRecentAction) {
      if (lastActionChannel == -1) {
        snprintf(line, sizeof(line), "ALL: %s", lastActionState ? "ACTIVE (ON)" : "SAFE (OFF)");
      } else {
        snprintf(line, sizeof(line), "%s (R%d): %s",
                 ACTUATOR_CODES[lastActionChannel],
                 lastActionChannel + 1,
                 lastActionState ? "ON" : "OFF");
      }
    } else {
      PressureData p;
      getPressureData(&p);
      if (p.isConnected) {
        snprintf(line, sizeof(line), "Air P: %5.1f mmHg", p.pressureMmhg);
      } else {
        snprintf(line, sizeof(line), "Air P: Connecting...");
      }
    }
    printLine(2, line);

    // Line 3: Uptime & RSSI
    unsigned long sec = millis() / 1000;
    unsigned long hrs = sec / 3600;
    unsigned long mins = (sec % 3600) / 60;
    unsigned long secs = sec % 60;
    int rssi = isConnected ? WiFi.RSSI() : 0;
    snprintf(line, sizeof(line), "Up:%02lu:%02lu:%02lu  %ddBm", hrs, mins, secs, rssi);
    printLine(3, line);
  }

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
