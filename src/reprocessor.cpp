#include "reprocessor.h"
#include "web_server.h"
#include "display.h"

// ============================================================================
// Configurable Durations (Seconds & Milliseconds)
// ============================================================================
static uint32_t s_tRoWash1Sec     = 40;  // Stage 1 duration
static uint32_t s_tH2o2WashSec    = 50;  // Stage 2 duration
static uint32_t s_tRoWash2Sec     = 40;  // Stage 3 duration
static uint32_t s_tPressurizeSec  = 10;  // Stage 4A duration
static uint32_t s_tHoldSec        = 15;  // Stage 4B duration
static uint32_t s_tVentSec        = 5;   // Stage 4C duration
static uint32_t s_tFillBloodSec   = 20;  // Stage 5 duration
static uint32_t s_tFbvMeasureSec  = 15;  // Stage 6 duration
static uint32_t s_tPaaPrimeSec    = 25;  // Stage 7 duration

static const uint32_t T_VALVE_PUMP_INTERLOCK_MS = 200;  // 100-250ms valve-before-pump
static const uint32_t T_PUMP_VALVE_INTERLOCK_MS = 150;  // 100-200ms pump-before-valve

// ============================================================================
// Internal FSM State Variables
// ============================================================================
static SemaphoreHandle_t s_fsmMutex = NULL;

static ReprocessorStage s_currentStage = STAGE_IDLE;
static SubStep s_currentSubStep = SUBSTEP_INIT;
static bool s_isRunning = false;
static bool s_isAutoCycle = false;
static bool s_isPaused = false;
static bool s_isAborted = false;
static uint8_t s_singleTargetStage = 0;

static unsigned long s_stageStartTimeMs = 0;
static unsigned long s_subStepStartTimeMs = 0;
static unsigned long s_cycleStartTimeMs = 0;
static unsigned long s_pauseStartTimeMs = 0;
static unsigned long s_totalPausedMs = 0;

static char s_statusMessage[64] = "System Idle - Ready";

// External hook to trigger WebSocket broadcast from reprocessor
extern void broadcastReprocessorStatus();

// ============================================================================
// Helper: Apply Active Relay Bitmask Safely
// ============================================================================
static void applyRelayPattern(bool r1, bool r2, bool r3, bool r4, bool r5,
                              bool r6, bool r7, bool r8, bool r9, bool r10) {
  setRelay(RELAY_PUMP1, r1);
  setRelay(RELAY_PUMP2, r2);
  setRelay(RELAY_PUMP3, r3);
  setRelay(RELAY_VALVE1, r4);
  setRelay(RELAY_VALVE2, r5);
  setRelay(RELAY_VALVE3, r6);
  setRelay(RELAY_VALVE4, r7);
  setRelay(RELAY_VALVE5, r8);
  setRelay(RELAY_VALVE6, r9);
  setRelay(RELAY_AIRPUMP, r10);
}

static void allRelaysOff() {
  setAllRelays(false);
}

// ============================================================================
// Names & Descriptions
// ============================================================================
const char* getStageName(ReprocessorStage stage) {
  switch (stage) {
    case STAGE_IDLE:                return "System Idle";
    case STAGE_RO_WASH_1:           return "Stage 1: RO Wash (Both Sides)";
    case STAGE_H2O2_WASH:           return "Stage 2: H2O2 Chemical Wash";
    case STAGE_RO_WASH_2:           return "Stage 3: Secondary RO Rinse";
    case STAGE_PRESSURE_PRESSURIZE: return "Stage 4A: Leak Test Pressurize";
    case STAGE_PRESSURE_HOLD:       return "Stage 4B: Leak Test Hold/Decay";
    case STAGE_PRESSURE_VENT:       return "Stage 4C: Leak Test Vent";
    case STAGE_RO_FILL_BLOOD:       return "Stage 5: RO Fill (Blood Side)";
    case STAGE_FBV_MEASURE:         return "Stage 6: FBV Volume Measure";
    case STAGE_PAA_PRIME:           return "Stage 7: PAA Sterilant Prime";
    case STAGE_COMPLETE:            return "Cycle Complete - Clean";
    case STAGE_PAUSED:              return "Cycle Paused";
    case STAGE_ABORTED:             return "Cycle Aborted / E-Stop";
    default:                        return "Unknown Stage";
  }
}

const char* getSubStepName(SubStep subStep) {
  switch (subStep) {
    case SUBSTEP_INIT:           return "Initializing";
    case SUBSTEP_VALVES_OPENING: return "Valves Energized (Interlock)";
    case SUBSTEP_PUMP_RUNNING:   return "Pump Running";
    case SUBSTEP_PUMP_STOPPING:  return "Pump Stopped (Interlock)";
    case SUBSTEP_AIR_LOCK_HOLD:  return "Chamber Sealed (Hold)";
    case SUBSTEP_VENTING:        return "Exhaust / Vent Open";
    case SUBSTEP_MEASURING:      return "Measuring Line Open";
    case SUBSTEP_DONE:           return "Step Complete";
    default:                     return "Active";
  }
}

static void getActiveActuatorsString(char* buf, size_t maxLen) {
  bool st[NUM_RELAYS] = {false};
  getAllRelayStates(st, NUM_RELAYS);

  buf[0] = '\0';
  bool first = true;
  for (int i = 0; i < NUM_RELAYS; i++) {
    if (st[i]) {
      if (!first) strncat(buf, ", ", maxLen - strlen(buf) - 1);
      strncat(buf, ACTUATOR_CODES[i], maxLen - strlen(buf) - 1);
      first = false;
    }
  }
  if (first) {
    strncpy(buf, "None (All OFF)", maxLen);
  }
}

// ============================================================================
// Initialization
// ============================================================================
void initReprocessor() {
  if (s_fsmMutex == NULL) {
    s_fsmMutex = xSemaphoreCreateMutex();
  }
  s_currentStage = STAGE_IDLE;
  s_currentSubStep = SUBSTEP_INIT;
  s_isRunning = false;
  s_isAutoCycle = false;
  s_isPaused = false;
  s_isAborted = false;
  allRelaysOff();
  Serial.println("[Reprocessor] Automated Dialyzer Reprocessor FSM initialized.");
}

// ============================================================================
// Control Interface
// ============================================================================
static void enterStage(ReprocessorStage stage) {
  s_currentStage = stage;
  s_currentSubStep = SUBSTEP_INIT;
  s_stageStartTimeMs = millis();
  s_subStepStartTimeMs = millis();
  Serial.printf("[Reprocessor] >>> Entering %s\n", getStageName(stage));
}

bool startAutoCycle() {
  if (s_fsmMutex == NULL) return false;
  if (xSemaphoreTake(s_fsmMutex, pdMS_TO_TICKS(100)) != pdTRUE) return false;

  allRelaysOff();
  s_isAutoCycle = true;
  s_singleTargetStage = 0;
  s_isRunning = true;
  s_isPaused = false;
  s_isAborted = false;
  s_cycleStartTimeMs = millis();
  s_totalPausedMs = 0;
  snprintf(s_statusMessage, sizeof(s_statusMessage), "Auto Reprocessing Cycle Running");

  enterStage(STAGE_RO_WASH_1);

  xSemaphoreGive(s_fsmMutex);
  broadcastReprocessorStatus();
  return true;
}

bool startSingleStage(int stageNumber) {
  if (s_fsmMutex == NULL) return false;
  if (stageNumber < 1 || stageNumber > 7) return false;

  if (xSemaphoreTake(s_fsmMutex, pdMS_TO_TICKS(100)) != pdTRUE) return false;

  allRelaysOff();
  s_isAutoCycle = false;
  s_singleTargetStage = stageNumber;
  s_isRunning = true;
  s_isPaused = false;
  s_isAborted = false;
  s_cycleStartTimeMs = millis();
  s_totalPausedMs = 0;
  snprintf(s_statusMessage, sizeof(s_statusMessage), "Running Single Stage %d", stageNumber);

  ReprocessorStage target;
  switch (stageNumber) {
    case 1: target = STAGE_RO_WASH_1; break;
    case 2: target = STAGE_H2O2_WASH; break;
    case 3: target = STAGE_RO_WASH_2; break;
    case 4: target = STAGE_PRESSURE_PRESSURIZE; break;
    case 5: target = STAGE_RO_FILL_BLOOD; break;
    case 6: target = STAGE_FBV_MEASURE; break;
    case 7: target = STAGE_PAA_PRIME; break;
    default: target = STAGE_IDLE; break;
  }

  enterStage(target);

  xSemaphoreGive(s_fsmMutex);
  broadcastReprocessorStatus();
  return true;
}

bool pauseCycle() {
  if (s_fsmMutex == NULL || !s_isRunning || s_isPaused) return false;
  if (xSemaphoreTake(s_fsmMutex, pdMS_TO_TICKS(100)) != pdTRUE) return false;

  s_isPaused = true;
  s_pauseStartTimeMs = millis();
  // Safe pause: stop all pumps immediately while keeping safe state
  setRelay(RELAY_PUMP1, false);
  setRelay(RELAY_PUMP2, false);
  setRelay(RELAY_PUMP3, false);
  setRelay(RELAY_AIRPUMP, false);
  snprintf(s_statusMessage, sizeof(s_statusMessage), "Cycle Paused by Operator");

  xSemaphoreGive(s_fsmMutex);
  broadcastReprocessorStatus();
  return true;
}

bool resumeCycle() {
  if (s_fsmMutex == NULL || !s_isRunning || !s_isPaused) return false;
  if (xSemaphoreTake(s_fsmMutex, pdMS_TO_TICKS(100)) != pdTRUE) return false;

  s_totalPausedMs += (millis() - s_pauseStartTimeMs);
  s_isPaused = false;
  snprintf(s_statusMessage, sizeof(s_statusMessage), "Cycle Resumed");

  // Re-enter sub-step to re-energize required pump if applicable
  s_subStepStartTimeMs = millis();

  xSemaphoreGive(s_fsmMutex);
  broadcastReprocessorStatus();
  return true;
}

bool abortCycle(const char* reason) {
  if (s_fsmMutex == NULL) return false;
  if (xSemaphoreTake(s_fsmMutex, pdMS_TO_TICKS(100)) != pdTRUE) return false;

  allRelaysOff();
  s_isRunning = false;
  s_isAutoCycle = false;
  s_isPaused = false;
  s_isAborted = true;
  s_currentStage = STAGE_ABORTED;
  s_currentSubStep = SUBSTEP_DONE;
  snprintf(s_statusMessage, sizeof(s_statusMessage), "Aborted: %s", reason ? reason : "Operator Abort");
  Serial.printf("[Reprocessor] Aborted: %s\n", s_statusMessage);

  xSemaphoreGive(s_fsmMutex);
  broadcastReprocessorStatus();
  return true;
}

void emergencyStop() {
  // 1. Direct hardware kill without waiting for mutex
  emergencyStopRelays();

  // 2. Update state variables
  s_isRunning = false;
  s_isAutoCycle = false;
  s_isPaused = false;
  s_isAborted = true;
  s_currentStage = STAGE_ABORTED;
  s_currentSubStep = SUBSTEP_DONE;
  strncpy(s_statusMessage, "EMERGENCY STOP TRIGGERED", sizeof(s_statusMessage));
  Serial.println("[Reprocessor] !!! EMERGENCY STOP TRIGGERED - ALL RELAYS SHUT DOWN !!!");

  broadcastReprocessorStatus();
}

ReprocessorStage getReprocessorStage() {
  return s_currentStage;
}

bool isReprocessorBusy() {
  return s_isRunning;
}

// ============================================================================
// State Duration & Progression Calculation
// ============================================================================
static uint32_t getStageDurationSec(ReprocessorStage stage) {
  switch (stage) {
    case STAGE_RO_WASH_1:           return s_tRoWash1Sec;
    case STAGE_H2O2_WASH:           return s_tH2o2WashSec;
    case STAGE_RO_WASH_2:           return s_tRoWash2Sec;
    case STAGE_PRESSURE_PRESSURIZE: return s_tPressurizeSec;
    case STAGE_PRESSURE_HOLD:       return s_tHoldSec;
    case STAGE_PRESSURE_VENT:       return s_tVentSec;
    case STAGE_RO_FILL_BLOOD:       return s_tFillBloodSec;
    case STAGE_FBV_MEASURE:         return s_tFbvMeasureSec;
    case STAGE_PAA_PRIME:           return s_tPaaPrimeSec;
    default:                        return 0;
  }
}

static uint32_t getTotalCycleDurationSec() {
  return s_tRoWash1Sec + s_tH2o2WashSec + s_tRoWash2Sec +
         s_tPressurizeSec + s_tHoldSec + s_tVentSec +
         s_tFillBloodSec + s_tFbvMeasureSec + s_tPaaPrimeSec;
}

// ============================================================================
// Telemetry Accessor
// ============================================================================
void getReprocessorStatus(ReprocessorStatus* st) {
  if (!st) return;

  st->stage = s_currentStage;
  st->subStep = s_currentSubStep;
  st->isRunning = s_isRunning;
  st->isAutoCycle = s_isAutoCycle;
  st->isPaused = s_isPaused;
  st->isAborted = s_isAborted;

  switch (s_currentStage) {
    case STAGE_RO_WASH_1:           st->stageNumber = 1; break;
    case STAGE_H2O2_WASH:           st->stageNumber = 2; break;
    case STAGE_RO_WASH_2:           st->stageNumber = 3; break;
    case STAGE_PRESSURE_PRESSURIZE:
    case STAGE_PRESSURE_HOLD:
    case STAGE_PRESSURE_VENT:       st->stageNumber = 4; break;
    case STAGE_RO_FILL_BLOOD:       st->stageNumber = 5; break;
    case STAGE_FBV_MEASURE:         st->stageNumber = 6; break;
    case STAGE_PAA_PRIME:           st->stageNumber = 7; break;
    default:                        st->stageNumber = 0; break;
  }

  uint32_t stageDur = getStageDurationSec(s_currentStage);
  st->stageTotalSec = stageDur;

  if (s_isRunning && !s_isPaused) {
    unsigned long now = millis();
    unsigned long elapsedMs = (now >= s_stageStartTimeMs) ? (now - s_stageStartTimeMs) : 0;
    st->stageElapsedSec = elapsedMs / 1000;

    unsigned long cycleElapsedMs = (now >= s_cycleStartTimeMs) ? (now - s_cycleStartTimeMs - s_totalPausedMs) : 0;
    st->cycleElapsedSec = cycleElapsedMs / 1000;
  } else {
    st->stageElapsedSec = 0;
    st->cycleElapsedSec = 0;
  }

  st->cycleTotalSec = s_isAutoCycle ? getTotalCycleDurationSec() : stageDur;

  if (st->cycleTotalSec > 0) {
    uint32_t pct = (st->cycleElapsedSec * 100) / st->cycleTotalSec;
    st->progressPercent = (pct > 100) ? 100 : (uint8_t)pct;
  } else {
    st->progressPercent = 0;
  }

  strncpy(st->stageName, getStageName(s_currentStage), sizeof(st->stageName));
  strncpy(st->subStepName, getSubStepName(s_currentSubStep), sizeof(st->subStepName));
  getActiveActuatorsString(st->activeActuators, sizeof(st->activeActuators));
  strncpy(st->statusMessage, s_statusMessage, sizeof(st->statusMessage));
}

String getReprocessorStatusJson() {
  ReprocessorStatus st;
  getReprocessorStatus(&st);

  String json = "{";
  json += "\"type\":\"reprocessor\",";
  json += "\"stage\":" + String((int)st.stage) + ",";
  json += "\"stageNumber\":" + String(st.stageNumber) + ",";
  json += "\"stageName\":\"" + String(st.stageName) + "\",";
  json += "\"subStep\":" + String((int)st.subStep) + ",";
  json += "\"subStepName\":\"" + String(st.subStepName) + "\",";
  json += "\"isRunning\":" + String(st.isRunning ? "true" : "false") + ",";
  json += "\"isAuto\":" + String(st.isAutoCycle ? "true" : "false") + ",";
  json += "\"isPaused\":" + String(st.isPaused ? "true" : "false") + ",";
  json += "\"isAborted\":" + String(st.isAborted ? "true" : "false") + ",";
  json += "\"stageElapsed\":" + String(st.stageElapsedSec) + ",";
  json += "\"stageTotal\":" + String(st.stageTotalSec) + ",";
  json += "\"cycleElapsed\":" + String(st.cycleElapsedSec) + ",";
  json += "\"cycleTotal\":" + String(st.cycleTotalSec) + ",";
  json += "\"progress\":" + String(st.progressPercent) + ",";
  json += "\"actuators\":\"" + String(st.activeActuators) + "\",";
  json += "\"message\":\"" + String(st.statusMessage) + "\"";
  json += "}";
  return json;
}

// ============================================================================
// Stage Transition Sequencer
// ============================================================================
static void nextStage() {
  if (!s_isAutoCycle) {
    // Single stage completed: return to IDLE
    allRelaysOff();
    s_isRunning = false;
    s_currentStage = STAGE_IDLE;
    s_currentSubStep = SUBSTEP_DONE;
    snprintf(s_statusMessage, sizeof(s_statusMessage), "Stage %d Complete - Idle", s_singleTargetStage);
    Serial.printf("[Reprocessor] %s\n", s_statusMessage);
    broadcastReprocessorStatus();
    return;
  }

  // Full Automated Sequence: 1 -> 2 -> 3 -> 4A -> 4B -> 4C -> 5 -> 6 -> 7 -> COMPLETE
  switch (s_currentStage) {
    case STAGE_RO_WASH_1:
      enterStage(STAGE_H2O2_WASH);
      break;

    case STAGE_H2O2_WASH:
      enterStage(STAGE_RO_WASH_2);
      break;

    case STAGE_RO_WASH_2:
      enterStage(STAGE_PRESSURE_PRESSURIZE);
      break;

    case STAGE_PRESSURE_PRESSURIZE:
      enterStage(STAGE_PRESSURE_HOLD);
      break;

    case STAGE_PRESSURE_HOLD:
      enterStage(STAGE_PRESSURE_VENT);
      break;

    case STAGE_PRESSURE_VENT:
      enterStage(STAGE_RO_FILL_BLOOD);
      break;

    case STAGE_RO_FILL_BLOOD:
      enterStage(STAGE_FBV_MEASURE);
      break;

    case STAGE_FBV_MEASURE:
      enterStage(STAGE_PAA_PRIME);
      break;

    case STAGE_PAA_PRIME:
      allRelaysOff();
      s_isRunning = false;
      s_isAutoCycle = false;
      s_currentStage = STAGE_COMPLETE;
      s_currentSubStep = SUBSTEP_DONE;
      snprintf(s_statusMessage, sizeof(s_statusMessage), "Reprocessing Complete - Dialyzer Cleaned & Stored");
      Serial.println("[Reprocessor] SUCCESS: Full reprocessing sequence complete!");
      break;

    default:
      allRelaysOff();
      s_isRunning = false;
      s_currentStage = STAGE_IDLE;
      s_currentSubStep = SUBSTEP_DONE;
      break;
  }

  broadcastReprocessorStatus();
}

// ============================================================================
// FreeRTOS Task: Asynchronous Finite State Machine (FSM)
// ============================================================================
void reprocessorTask(void* pvParameters) {
  Serial.println("[FreeRTOS] Reprocessor Task running on Core " + String(xPortGetCoreID()));

  TickType_t lastWakeTime = xTaskGetTickCount();
  const TickType_t stepFrequency = pdMS_TO_TICKS(50); // 50 ms loop tick (20 Hz)
  unsigned long lastBroadcastTime = 0;

  for (;;) {
    vTaskDelayUntil(&lastWakeTime, stepFrequency);

    if (!s_isRunning || s_isPaused) {
      continue;
    }

    unsigned long now = millis();

    // ------------------------------------------------------------------------
    // Stage 1: Initial RO Flush (Both Sides)
    // Active Relays: R1 (Pump 1), R4 (S1), R7 (S4), R9 (S6)
    // ------------------------------------------------------------------------
    if (s_currentStage == STAGE_RO_WASH_1) {
      switch (s_currentSubStep) {
        case SUBSTEP_INIT:
          // 1. Energize valves S1, S4, S6 (R4, R7, R9 = HIGH)
          applyRelayPattern(false, false, false, true, false, false, true, false, true, false);
          s_currentSubStep = SUBSTEP_VALVES_OPENING;
          s_subStepStartTimeMs = now;
          broadcastReprocessorStatus();
          break;

        case SUBSTEP_VALVES_OPENING:
          // Interlock delay: wait 100-250 ms before turning pump ON
          if (now - s_subStepStartTimeMs >= T_VALVE_PUMP_INTERLOCK_MS) {
            setRelay(RELAY_PUMP1, true); // Start RO Pump 1 (R1 = HIGH)
            s_currentSubStep = SUBSTEP_PUMP_RUNNING;
            s_subStepStartTimeMs = now;
            broadcastReprocessorStatus();
          }
          break;

        case SUBSTEP_PUMP_RUNNING:
          // Run pump for duration T_RO_WASH_1
          if (now - s_subStepStartTimeMs >= (s_tRoWash1Sec * 1000)) {
            setRelay(RELAY_PUMP1, false); // Stop pump (R1 = LOW)
            s_currentSubStep = SUBSTEP_PUMP_STOPPING;
            s_subStepStartTimeMs = now;
            broadcastReprocessorStatus();
          }
          break;

        case SUBSTEP_PUMP_STOPPING:
          // Interlock delay: wait 100-200 ms before closing valves
          if (now - s_subStepStartTimeMs >= T_PUMP_VALVE_INTERLOCK_MS) {
            allRelaysOff();
            s_currentSubStep = SUBSTEP_DONE;
            nextStage();
          }
          break;

        default:
          break;
      }
    }

    // ------------------------------------------------------------------------
    // Stage 2: H2O2 Chemical Wash
    // Active Relays: R2 (Pump 2), R5 (S2), R7 (S4), R9 (S6)
    // ------------------------------------------------------------------------
    else if (s_currentStage == STAGE_H2O2_WASH) {
      switch (s_currentSubStep) {
        case SUBSTEP_INIT:
          // 1. Open valves S2, S4, S6 (R5, R7, R9 = HIGH)
          applyRelayPattern(false, false, false, false, true, false, true, false, true, false);
          s_currentSubStep = SUBSTEP_VALVES_OPENING;
          s_subStepStartTimeMs = now;
          broadcastReprocessorStatus();
          break;

        case SUBSTEP_VALVES_OPENING:
          if (now - s_subStepStartTimeMs >= T_VALVE_PUMP_INTERLOCK_MS) {
            setRelay(RELAY_PUMP2, true); // Start chemical pump (R2 = HIGH)
            s_currentSubStep = SUBSTEP_PUMP_RUNNING;
            s_subStepStartTimeMs = now;
            broadcastReprocessorStatus();
          }
          break;

        case SUBSTEP_PUMP_RUNNING:
          if (now - s_subStepStartTimeMs >= (s_tH2o2WashSec * 1000)) {
            setRelay(RELAY_PUMP2, false); // Turn off pump (R2 = LOW)
            s_currentSubStep = SUBSTEP_PUMP_STOPPING;
            s_subStepStartTimeMs = now;
            broadcastReprocessorStatus();
          }
          break;

        case SUBSTEP_PUMP_STOPPING:
          if (now - s_subStepStartTimeMs >= T_PUMP_VALVE_INTERLOCK_MS) {
            allRelaysOff();
            s_currentSubStep = SUBSTEP_DONE;
            nextStage();
          }
          break;

        default:
          break;
      }
    }

    // ------------------------------------------------------------------------
    // Stage 3: Secondary RO Rinse
    // Active Relays: R1 (Pump 1), R4 (S1), R7 (S4), R9 (S6)
    // ------------------------------------------------------------------------
    else if (s_currentStage == STAGE_RO_WASH_2) {
      switch (s_currentSubStep) {
        case SUBSTEP_INIT:
          // Open valves S1, S4, S6
          applyRelayPattern(false, false, false, true, false, false, true, false, true, false);
          s_currentSubStep = SUBSTEP_VALVES_OPENING;
          s_subStepStartTimeMs = now;
          broadcastReprocessorStatus();
          break;

        case SUBSTEP_VALVES_OPENING:
          if (now - s_subStepStartTimeMs >= T_VALVE_PUMP_INTERLOCK_MS) {
            setRelay(RELAY_PUMP1, true); // Energize R1 (Pump 1)
            s_currentSubStep = SUBSTEP_PUMP_RUNNING;
            s_subStepStartTimeMs = now;
            broadcastReprocessorStatus();
          }
          break;

        case SUBSTEP_PUMP_RUNNING:
          if (now - s_subStepStartTimeMs >= (s_tRoWash2Sec * 1000)) {
            setRelay(RELAY_PUMP1, false); // De-energize R1
            s_currentSubStep = SUBSTEP_PUMP_STOPPING;
            s_subStepStartTimeMs = now;
            broadcastReprocessorStatus();
          }
          break;

        case SUBSTEP_PUMP_STOPPING:
          if (now - s_subStepStartTimeMs >= T_PUMP_VALVE_INTERLOCK_MS) {
            allRelaysOff(); // Close valves
            s_currentSubStep = SUBSTEP_DONE;
            nextStage();
          }
          break;

        default:
          break;
      }
    }

    // ------------------------------------------------------------------------
    // Stage 4A: Pressure Test: Pressurization
    // Active Relays: R8 (S5), R10 (Air Pump)
    // ------------------------------------------------------------------------
    else if (s_currentStage == STAGE_PRESSURE_PRESSURIZE) {
      switch (s_currentSubStep) {
        case SUBSTEP_INIT:
          // Open isolation valve S5 (R8 = HIGH)
          applyRelayPattern(false, false, false, false, false, false, false, true, false, false);
          s_currentSubStep = SUBSTEP_VALVES_OPENING;
          s_subStepStartTimeMs = now;
          broadcastReprocessorStatus();
          break;

        case SUBSTEP_VALVES_OPENING:
          if (now - s_subStepStartTimeMs >= 150) {
            setRelay(RELAY_AIRPUMP, true); // Start air pump (R10 = HIGH)
            s_currentSubStep = SUBSTEP_PUMP_RUNNING;
            s_subStepStartTimeMs = now;
            broadcastReprocessorStatus();
          }
          break;

        case SUBSTEP_PUMP_RUNNING:
          if (now - s_subStepStartTimeMs >= (s_tPressurizeSec * 1000)) {
            s_currentSubStep = SUBSTEP_DONE;
            nextStage();
          }
          break;

        default:
          break;
      }
    }

    // ------------------------------------------------------------------------
    // Stage 4B: Pressure Test: Stabilization / Decay (Air Lock Hold)
    // Active Relays: NONE (All R1 - R10 OFF)
    // ------------------------------------------------------------------------
    else if (s_currentStage == STAGE_PRESSURE_HOLD) {
      switch (s_currentSubStep) {
        case SUBSTEP_INIT:
          // Turn off air pump (R10 = LOW) and isolation valve S5 (R8 = LOW)
          allRelaysOff();
          s_currentSubStep = SUBSTEP_AIR_LOCK_HOLD;
          s_subStepStartTimeMs = now;
          broadcastReprocessorStatus();
          break;

        case SUBSTEP_AIR_LOCK_HOLD:
          // Hold chamber sealed for test window T_HOLD
          if (now - s_subStepStartTimeMs >= (s_tHoldSec * 1000)) {
            s_currentSubStep = SUBSTEP_DONE;
            nextStage();
          }
          break;

        default:
          break;
      }
    }

    // ------------------------------------------------------------------------
    // Stage 4C: Pressure Test: Vent / Depressurize
    // Active Relays: R9 (S6 Vent Open)
    // ------------------------------------------------------------------------
    else if (s_currentStage == STAGE_PRESSURE_VENT) {
      switch (s_currentSubStep) {
        case SUBSTEP_INIT:
          // Open exhaust valve S6 (R9 = HIGH)
          applyRelayPattern(false, false, false, false, false, false, false, false, true, false);
          s_currentSubStep = SUBSTEP_VENTING;
          s_subStepStartTimeMs = now;
          broadcastReprocessorStatus();
          break;

        case SUBSTEP_VENTING:
          if (now - s_subStepStartTimeMs >= (s_tVentSec * 1000)) {
            setRelay(RELAY_VALVE6, false); // Close exhaust valve S6
            s_currentSubStep = SUBSTEP_DONE;
            nextStage();
          }
          break;

        default:
          break;
      }
    }

    // ------------------------------------------------------------------------
    // Stage 5: RO Water Fill (Blood Compartment Pre-fill)
    // Active Relays: R1 (Pump 1), R4 (S1), R9 (S6)
    // ------------------------------------------------------------------------
    else if (s_currentStage == STAGE_RO_FILL_BLOOD) {
      switch (s_currentSubStep) {
        case SUBSTEP_INIT:
          // Open inlet valve S1 and drain/vent S6 (R4 = HIGH, R9 = HIGH)
          applyRelayPattern(false, false, false, true, false, false, false, false, true, false);
          s_currentSubStep = SUBSTEP_VALVES_OPENING;
          s_subStepStartTimeMs = now;
          broadcastReprocessorStatus();
          break;

        case SUBSTEP_VALVES_OPENING:
          if (now - s_subStepStartTimeMs >= T_VALVE_PUMP_INTERLOCK_MS) {
            setRelay(RELAY_PUMP1, true); // Turn on RO Pump 1 (R1 = HIGH)
            s_currentSubStep = SUBSTEP_PUMP_RUNNING;
            s_subStepStartTimeMs = now;
            broadcastReprocessorStatus();
          }
          break;

        case SUBSTEP_PUMP_RUNNING:
          if (now - s_subStepStartTimeMs >= (s_tFillBloodSec * 1000)) {
            setRelay(RELAY_PUMP1, false); // Cut off pump
            s_currentSubStep = SUBSTEP_PUMP_STOPPING;
            s_subStepStartTimeMs = now;
            broadcastReprocessorStatus();
          }
          break;

        case SUBSTEP_PUMP_STOPPING:
          if (now - s_subStepStartTimeMs >= T_PUMP_VALVE_INTERLOCK_MS) {
            allRelaysOff(); // Close valves S1, S6
            s_currentSubStep = SUBSTEP_DONE;
            nextStage();
          }
          break;

        default:
          break;
      }
    }

    // ------------------------------------------------------------------------
    // Stage 6: Fiber Bundle Volume (FBV) Test Measurement
    // Active Relays: R6 (S3 Open)
    // ------------------------------------------------------------------------
    else if (s_currentStage == STAGE_FBV_MEASURE) {
      switch (s_currentSubStep) {
        case SUBSTEP_INIT:
          // Open measurement line valve S3 (R6 = HIGH)
          applyRelayPattern(false, false, false, false, false, true, false, false, false, false);
          s_currentSubStep = SUBSTEP_MEASURING;
          s_subStepStartTimeMs = now;
          broadcastReprocessorStatus();
          break;

        case SUBSTEP_MEASURING:
          if (now - s_subStepStartTimeMs >= (s_tFbvMeasureSec * 1000)) {
            setRelay(RELAY_VALVE3, false); // Close line valve S3
            s_currentSubStep = SUBSTEP_DONE;
            nextStage();
          }
          break;

        default:
          break;
      }
    }

    // ------------------------------------------------------------------------
    // Stage 7: Renalin / PAA Sanitization & Storage Prime
    // Active Relays: R3 (Pump 3), R6 (S3), R7 (S4)
    // ------------------------------------------------------------------------
    else if (s_currentStage == STAGE_PAA_PRIME) {
      switch (s_currentSubStep) {
        case SUBSTEP_INIT:
          // Open routing valves S3 and S4 (R6 = HIGH, R7 = HIGH)
          applyRelayPattern(false, false, false, false, false, true, true, false, false, false);
          s_currentSubStep = SUBSTEP_VALVES_OPENING;
          s_subStepStartTimeMs = now;
          broadcastReprocessorStatus();
          break;

        case SUBSTEP_VALVES_OPENING:
          if (now - s_subStepStartTimeMs >= T_VALVE_PUMP_INTERLOCK_MS) {
            setRelay(RELAY_PUMP3, true); // Start PAA Pump 3 (R3 = HIGH)
            s_currentSubStep = SUBSTEP_PUMP_RUNNING;
            s_subStepStartTimeMs = now;
            broadcastReprocessorStatus();
          }
          break;

        case SUBSTEP_PUMP_RUNNING:
          if (now - s_subStepStartTimeMs >= (s_tPaaPrimeSec * 1000)) {
            setRelay(RELAY_PUMP3, false); // Turn off pump (R3 = LOW)
            s_currentSubStep = SUBSTEP_PUMP_STOPPING;
            s_subStepStartTimeMs = now;
            broadcastReprocessorStatus();
          }
          break;

        case SUBSTEP_PUMP_STOPPING:
          if (now - s_subStepStartTimeMs >= T_PUMP_VALVE_INTERLOCK_MS) {
            allRelaysOff(); // Close remaining valves (R6 = LOW, R7 = LOW)
            s_currentSubStep = SUBSTEP_DONE;
            nextStage();
          }
          break;

        default:
          break;
      }
    }

    // Periodic live progress push (every 1 second) while running
    if (now - lastBroadcastTime >= 1000) {
      lastBroadcastTime = now;
      broadcastReprocessorStatus();
    }
  }
}

