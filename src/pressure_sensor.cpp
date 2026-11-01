#include "pressure_sensor.h"

// ============================================================================
// Internal State & Mutex
// ============================================================================
static SemaphoreHandle_t s_sensorMutex = NULL;

static PressureData s_latestData = {
  .isConnected   = false,
  .rawValue      = 0,
  .tareOffset    = 0,
  .deltaRaw      = 0,
  .pressureKPa   = 0.0f,
  .pressureMmhg  = 0.0f,
  .pressurePsi   = 0.0f,
  .timestampMs   = 0
};

// Default scale factor for typical 40kPa piezoresistive sensor on HX710B
// (Approx 20,000 counts per 1 kPa = ~2,666 counts per 1 mmHg)
static float s_countsPerKpa = 20000.0f;
static float s_filteredRaw = 0.0f;
static bool s_hasValidReading = false;

// ============================================================================
// Low-Level Driver
// ============================================================================
void initPressureSensor() {
  if (s_sensorMutex == NULL) {
    s_sensorMutex = xSemaphoreCreateMutex();
  }

  pinMode(HX710_DOUT_PIN, INPUT);
  pinMode(HX710_SCLK_PIN, OUTPUT);
  digitalWrite(HX710_SCLK_PIN, LOW);

  Serial.printf("[Pressure] HX710B initialized on DOUT=GPIO%d, SCLK=GPIO%d\n",
                HX710_DOUT_PIN, HX710_SCLK_PIN);

  // Perform quick initial tare if sensor is ready
  tarePressureSensor(4);
}

bool isPressureSensorReady() {
  return (digitalRead(HX710_DOUT_PIN) == LOW);
}

bool readPressureRaw(long* rawOut, uint32_t timeoutMs) {
  if (!rawOut) return false;

  unsigned long start = millis();
  // Wait for DOUT to go LOW (data ready signal from HX710B)
  while (digitalRead(HX710_DOUT_PIN) == HIGH) {
    if (millis() - start > timeoutMs) {
      return false; // Timeout: sensor disconnected or converting
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  }

  // 24 clock pulses: read MSB first
  // Standard microsecond delays (no spinlock or interrupt blocking)
  unsigned long raw = 0;
  for (int i = 0; i < 24; i++) {
    digitalWrite(HX710_SCLK_PIN, HIGH);
    delayMicroseconds(1);
    raw = (raw << 1);
    digitalWrite(HX710_SCLK_PIN, LOW);
    delayMicroseconds(1);
    if (digitalRead(HX710_DOUT_PIN)) {
      raw |= 1;
    }
  }

  // 25th clock pulse: Selects differential input and pulls DOUT back to HIGH
  digitalWrite(HX710_SCLK_PIN, HIGH);
  delayMicroseconds(1);
  digitalWrite(HX710_SCLK_PIN, LOW);
  delayMicroseconds(1);

  // 24-bit two's complement sign extension
  long signedVal = (long)raw;
  if (raw & 0x800000) {
    signedVal |= 0xFF000000;
  }

  *rawOut = signedVal;
  return true;
}

bool tarePressureSensor(uint8_t samples) {
  if (samples == 0) samples = 1;

  long sum = 0;
  uint8_t count = 0;
  unsigned long start = millis();

  // Non-blocking sample collection with maximum 600ms total budget
  while (count < samples && (millis() - start < 600)) {
    if (isPressureSensorReady()) {
      long sample = 0;
      if (readPressureRaw(&sample, 10)) {
        sum += sample;
        count++;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(15));
  }

  if (count == 0) {
    return false;
  }

  long newTare = sum / count;

  if (s_sensorMutex != NULL && xSemaphoreTake(s_sensorMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    s_latestData.tareOffset = newTare;
    s_filteredRaw = (float)newTare;
    xSemaphoreGive(s_sensorMutex);
    Serial.printf("[Pressure] Tare successful. Offset: %ld counts (%d samples)\n", newTare, count);
    return true;
  }

  return false;
}

void setPressureScale(float countsPerKpa) {
  if (countsPerKpa > 0.1f) {
    s_countsPerKpa = countsPerKpa;
  }
}

float getPressureScale() {
  return s_countsPerKpa;
}

void updatePressureSensor() {
  // Non-blocking check: only read if DOUT is LOW (conversion is ready)
  if (!isPressureSensorReady()) {
    return;
  }

  long raw = 0;
  bool ok = readPressureRaw(&raw, 5);

  if (s_sensorMutex != NULL && xSemaphoreTake(s_sensorMutex, pdMS_TO_TICKS(30)) == pdTRUE) {
    s_latestData.isConnected = ok;

    if (ok) {
      // Exponential moving average filter (alpha = 0.25)
      if (!s_hasValidReading) {
        s_filteredRaw = (float)raw;
        s_hasValidReading = true;
      } else {
        s_filteredRaw = (s_filteredRaw * 0.75f) + ((float)raw * 0.25f);
      }

      s_latestData.rawValue = raw;
      s_latestData.deltaRaw = (long)s_filteredRaw - s_latestData.tareOffset;

      float kpa = (float)s_latestData.deltaRaw / s_countsPerKpa;
      s_latestData.pressureKPa  = kpa;
      s_latestData.pressureMmhg = kpa * 7.50062f; // 1 kPa = 7.50062 mmHg
      s_latestData.pressurePsi  = kpa * 0.145038f; // 1 kPa = 0.145038 psi
      s_latestData.timestampMs  = millis();
    }

    xSemaphoreGive(s_sensorMutex);
  }
}

void getPressureData(PressureData* dataOut) {
  if (!dataOut) return;

  if (s_sensorMutex != NULL && xSemaphoreTake(s_sensorMutex, pdMS_TO_TICKS(40)) == pdTRUE) {
    *dataOut = s_latestData;
    xSemaphoreGive(s_sensorMutex);
  } else {
    *dataOut = s_latestData;
  }
}

String getPressureJson() {
  PressureData d;
  getPressureData(&d);

  String json = "{";
  json += "\"type\":\"pressure\",";
  json += "\"connected\":" + String(d.isConnected ? "true" : "false") + ",";
  json += "\"raw\":" + String(d.rawValue) + ",";
  json += "\"tare\":" + String(d.tareOffset) + ",";
  json += "\"delta\":" + String(d.deltaRaw) + ",";
  json += "\"kpa\":" + String(d.pressureKPa, 2) + ",";
  json += "\"mmhg\":" + String(d.pressureMmhg, 1) + ",";
  json += "\"psi\":" + String(d.pressurePsi, 2) + ",";
  json += "\"scale\":" + String(s_countsPerKpa, 1) + ",";
  json += "\"ageMs\":" + String(millis() - d.timestampMs);
  json += "}";
  return json;
}
