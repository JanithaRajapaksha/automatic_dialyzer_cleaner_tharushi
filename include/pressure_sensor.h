#ifndef PRESSURE_SENSOR_H
#define PRESSURE_SENSOR_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "config.h"

// ============================================================================
// HX710B Pressure Data Structure
// ============================================================================
struct PressureData {
  bool isConnected;       // True if sensor is actively responding on DOUT
  long rawValue;          // Current raw 24-bit ADC reading
  long tareOffset;        // Atmospheric baseline (zero offset)
  long deltaRaw;          // Net reading: rawValue - tareOffset
  float pressureKPa;      // Gauge pressure in kilopascals (kPa)
  float pressureMmhg;     // Gauge pressure in millimeters of mercury (mmHg)
  float pressurePsi;      // Gauge pressure in pounds per square inch (psi)
  unsigned long timestampMs; // Millis timestamp of last successful read
};

// ============================================================================
// Pressure Sensor API
// ============================================================================

// Initialize GPIO pins (DOUT=32, SCLK=14), create RTOS mutex, and sample initial tare
void initPressureSensor();

// Check if HX710B DOUT is LOW (indicates new 24-bit conversion ready)
bool isPressureSensorReady();

// Read raw 24-bit two's complement value from HX710B (timeout protected)
bool readPressureRaw(long* rawOut, uint32_t timeoutMs = 150);

// Tare the sensor: averages multiple samples to establish 0 kPa atmospheric reference
bool tarePressureSensor(uint8_t samples = 10);

// Set custom calibration scale factor (counts per kPa)
void setPressureScale(float countsPerKpa);

// Get current calibration scale factor
float getPressureScale();

// Thread-safe fetch of latest pressure telemetry
void getPressureData(PressureData* dataOut);

// Get pressure telemetry formatted as a JSON string
String getPressureJson();

// Periodic background updater (runs in system monitor or dedicated loop)
void updatePressureSensor();

#endif // PRESSURE_SENSOR_H

