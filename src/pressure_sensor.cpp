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
  .rawMmhg       = 0.0f,
  .pressureKPa   = 0.0f,
  .pressureMmhg  = 0.0f,
  .pressurePsi   = 0.0f,
  .kalmanGain    = 0.0f,
  .timestampMs   = 0
};

// Default scale factor for typical 40kPa piezoresistive sensor on HX710B
// (Approx 20,000 counts per 1 kPa = ~2,666 counts per 1 mmHg)
static float s_countsPerKpa = 20000.0f;

// ============================================================================
// Adaptive 1D Kalman Filter
// Suppresses ADC & electrical noise during steady holding while tracking rapid
// pressure surges (e.g. pump start / valve actuation) with near-zero latency.
// ============================================================================
class AdaptiveKalmanFilter {
private:
  float x;          // State estimate (mmHg)
  float p;          // Estimation error covariance
  float q;          // Base process noise covariance (how quickly real pressure changes)
  float r;          // Measurement noise covariance (sensor / ADC noise variance)
  float k;          // Current Kalman gain (0.0 to 1.0)
  bool initialized;

public:
  AdaptiveKalmanFilter(float processNoise = 0.08f, float measurementNoise = 2.5f)
    : x(0.0f), p(1.0f), q(processNoise), r(measurementNoise), k(0.0f), initialized(false) {}

  void reset(float initialVal = 0.0f) {
    x = initialVal;
    p = 1.0f;
    k = 0.0f;
    initialized = true;
  }

  void setParameters(float processNoise, float measurementNoise) {
    if (processNoise > 0.0001f) q = processNoise;
    if (measurementNoise > 0.0001f) r = measurementNoise;
  }

  float update(float measurement) {
    if (!initialized) {
      x = measurement;
      p = 1.0f;
      initialized = true;
      return x;
    }

    // Innovation residual
    float residual = measurement - x;
    float absRes = fabsf(residual);

    // Adaptive Process Noise:
    // If pressure jumps significantly (> 2.0 mmHg in 100ms), pump or valve activated.
    // Scale Q dynamically to snap immediately to the new pressure without lag.
    float dynamicQ = q;
    if (absRes > 2.0f) {
      dynamicQ += absRes * 0.75f;
    }

    // Time update (prediction)
    float p_pred = p + dynamicQ;

    // Measurement update (correction)
    k = p_pred / (p_pred + r);
    x = x + k * residual;
    p = (1.0f - k) * p_pred;

    return x;
  }

  float getState() const { return x; }
  float getGain() const { return k; }
};

static AdaptiveKalmanFilter s_kalman(0.08f, 2.5f);

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
    s_kalman.reset(0.0f);
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

void setKalmanParameters(float processNoise, float measurementNoise) {
  s_kalman.setParameters(processNoise, measurementNoise);
}

void resetKalmanFilter(float initialMmhg) {
  s_kalman.reset(initialMmhg);
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
      s_latestData.rawValue = raw;
      s_latestData.deltaRaw = raw - s_latestData.tareOffset;

      // Instantaneous unfiltered values
      float rawKpa = (float)s_latestData.deltaRaw / s_countsPerKpa;
      float rawMmhg = rawKpa * 7.50062f; // 1 kPa = 7.50062 mmHg
      s_latestData.rawMmhg = rawMmhg;

      // Apply 1D Adaptive Kalman Filter (removes ADC/electrical jitter)
      float filteredMmhg = s_kalman.update(rawMmhg);
      s_latestData.pressureMmhg = filteredMmhg;
      s_latestData.pressureKPa  = filteredMmhg / 7.50062f;
      s_latestData.pressurePsi  = s_latestData.pressureKPa * 0.145038f;
      s_latestData.kalmanGain   = s_kalman.getGain();
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
  json += "\"rawMmhg\":" + String(d.rawMmhg, 1) + ",";
  json += "\"psi\":" + String(d.pressurePsi, 2) + ",";
  json += "\"gain\":" + String(d.kalmanGain, 2) + ",";
  json += "\"scale\":" + String(s_countsPerKpa, 1) + ",";
  json += "\"ageMs\":" + String(millis() - d.timestampMs);
  json += "}";
  return json;
}
