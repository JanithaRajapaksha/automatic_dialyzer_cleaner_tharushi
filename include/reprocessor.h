#ifndef REPROCESSOR_H
#define REPROCESSOR_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "config.h"
#include "relays.h"

// ============================================================================
// Process Stage Enumeration
// ============================================================================
enum ReprocessorStage {
  STAGE_IDLE                = 0,
  STAGE_RO_WASH_1           = 1,   // Stage 1: Initial RO Flush (Both Sides)
  STAGE_H2O2_WASH           = 2,   // Stage 2: H2O2 Chemical Wash
  STAGE_RO_WASH_2           = 3,   // Stage 3: Secondary RO Rinse
  STAGE_PRESSURE_PRESSURIZE = 41,  // Stage 4A: Pressure Test: Pressurization
  STAGE_PRESSURE_HOLD       = 42,  // Stage 4B: Pressure Test: Stabilization / Decay
  STAGE_PRESSURE_VENT       = 43,  // Stage 4C: Pressure Test: Vent / Depressurize
  STAGE_RO_FILL_BLOOD       = 5,   // Stage 5: RO Water Fill (Blood Side / FBV)
  STAGE_FBV_MEASURE         = 6,   // Stage 6: FBV Test Measurement
  STAGE_PAA_PRIME           = 7,   // Stage 7: PAA / Renalin Sanitization & Storage
  STAGE_COMPLETE            = 8,   // All stages complete, entering IDLE
  STAGE_PAUSED              = 98,  // Cycle paused
  STAGE_ABORTED             = 99   // Cycle aborted / E-Stop
};

// ============================================================================
// Internal Sub-Step Timing & Interlocking
// ============================================================================
enum SubStep {
  SUBSTEP_INIT            = 0,  // Pre-configuring state
  SUBSTEP_VALVES_OPENING  = 1,  // Directional valves energized (interlock delay)
  SUBSTEP_PUMP_RUNNING    = 2,  // Pump energized and running for stage duration
  SUBSTEP_PUMP_STOPPING   = 3,  // Pump stopped, interlock delay before closing valves
  SUBSTEP_AIR_LOCK_HOLD   = 4,  // Sealed chamber hold (Stage 4B)
  SUBSTEP_VENTING         = 5,  // Exhaust valve open (Stage 4C)
  SUBSTEP_MEASURING       = 6,  // Optical/volumetric measurement line open (Stage 6)
  SUBSTEP_DONE            = 7   // Stage transition complete
};

// ============================================================================
// Reprocessor Telemetry Structure
// ============================================================================
struct ReprocessorStatus {
  ReprocessorStage stage;
  SubStep subStep;
  bool isRunning;
  bool isAutoCycle;
  bool isPaused;
  bool isAborted;
  uint8_t stageNumber;      // 0 to 7
  uint32_t stageElapsedSec;
  uint32_t stageTotalSec;
  uint32_t cycleElapsedSec;
  uint32_t cycleTotalSec;
  uint8_t progressPercent;
  char stageName[48];
  char subStepName[48];
  char activeActuators[40];
  char statusMessage[64];
};

// ============================================================================
// Reprocessor Control API
// ============================================================================
void initReprocessor();

// Start full automated reprocessing cycle (Stages 1 through 7)
bool startAutoCycle();

// Start a single process stage (1 to 7)
bool startSingleStage(int stageNumber);

// Pause active cycle (safely stops pumps while maintaining valves or returns safe hold)
bool pauseCycle();

// Resume a paused cycle
bool resumeCycle();

// Abort active cycle safely (shuts down pumps and valves, transitions to IDLE)
bool abortCycle(const char* reason = "Manual Abort");

// Immediate Emergency E-Stop (clears hardware output port directly)
void emergencyStop();

// Thread-safe telemetry getters
void getReprocessorStatus(ReprocessorStatus* statusOut);
String getReprocessorStatusJson();
ReprocessorStage getReprocessorStage();
bool isReprocessorBusy();

// FreeRTOS Task function
void reprocessorTask(void* pvParameters);

// Stage helper strings
const char* getStageName(ReprocessorStage stage);
const char* getSubStepName(SubStep subStep);

#endif // REPROCESSOR_H

