/**
 * ============================================================================
 * Project : Automatic Dialyzer Cleaner - Embedded Control System
 * Hardware: ESP32-WROOM-32 (10-Channel Relay Controller)
 * OS      : FreeRTOS (ESP-IDF)
 * ============================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "config.h"
#include "relays.h"
#include "web_server.h"

// Task handles for FreeRTOS
TaskHandle_t webServerTaskHandle = NULL;
TaskHandle_t systemMonitorTaskHandle = NULL;

// ----------------------------------------------------------------------------
// Connect to Wi-Fi
// ----------------------------------------------------------------------------
void connectWiFi() {
  Serial.printf("[WiFi] Connecting to %s...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    vTaskDelay(pdMS_TO_TICKS(500));
    Serial.print(".");
    attempts++;
  }

  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WiFi] Connected successfully!");
    Serial.print("[WiFi] IP Address: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("[WiFi] Connection timed out. Monitor task will retry.");
  }
}

// ----------------------------------------------------------------------------
// FreeRTOS Task: Web Server Handler
// Dedicated task pinned to Core 0 for handling HTTP network traffic
// ----------------------------------------------------------------------------
void webServerTask(void* pvParameters) {
  Serial.println("[FreeRTOS] Web Server Task started on Core " + String(xPortGetCoreID()));

  for (;;) {
    handleWebServerClient();
    // Yield CPU to the scheduler and watchdog (2 ms delay)
    vTaskDelay(pdMS_TO_TICKS(2));
  }
}

// ----------------------------------------------------------------------------
// FreeRTOS Task: System Monitor & Watchdog
// Runs on Core 1: monitors WiFi status, memory health, and handles auto-reconnect
// ----------------------------------------------------------------------------
void systemMonitorTask(void* pvParameters) {
  Serial.println("[FreeRTOS] System Monitor Task started on Core " + String(xPortGetCoreID()));

  for (;;) {
    // Check Wi-Fi connectivity and attempt reconnection if dropped
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[Monitor] Wi-Fi lost. Attempting reconnection...");
      WiFi.reconnect();
    }

    // Diagnostic logging
    Serial.printf("[Monitor] System: %s | Free Heap: %lu bytes | WiFi RSSI: %d dBm\n",
                  SYSTEM_NAME,
                  ESP.getFreeHeap(),
                  WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0);

    // Sleep for 10 seconds before next check
    vTaskDelay(pdMS_TO_TICKS(10000));
  }
}

// ----------------------------------------------------------------------------
// Setup
// ----------------------------------------------------------------------------
void setup() {
  // 1. Initialize relay hardware FIRST so all channels are safely OFF immediately
  initRelays();

  Serial.begin(115200);
  delay(500);

  Serial.println("\n==================================================");
  Serial.printf(" %s v%s\n", SYSTEM_NAME, SYSTEM_VERSION);
  Serial.println("==================================================");
  Serial.println("[Init] Relay channels initialized to safe OFF state.");

  // 2. Connect to Wi-Fi network
  connectWiFi();

  // 3. Initialize HTTP Web Server and endpoints
  initWebServer();

  // 4. Create FreeRTOS Tasks
  Serial.println("[Init] Spawning FreeRTOS tasks...");

  // Web Server Task pinned to Core 0 (Networking Core)
  xTaskCreatePinnedToCore(
    webServerTask,
    "WebServerTask",
    WEBSERVER_TASK_STACK_SIZE,
    NULL,
    WEBSERVER_TASK_PRIORITY,
    &webServerTaskHandle,
    WEBSERVER_TASK_CORE
  );

  // System Monitor Task pinned to Core 1 (Application Core)
  xTaskCreatePinnedToCore(
    systemMonitorTask,
    "SystemMonitorTask",
    MONITOR_TASK_STACK_SIZE,
    NULL,
    MONITOR_TASK_PRIORITY,
    &systemMonitorTaskHandle,
    MONITOR_TASK_CORE
  );

  Serial.println("[Init] Initialization complete. System ready.");
}

// ----------------------------------------------------------------------------
// Main Arduino Loop
// Handled by FreeRTOS scheduler; yields CPU to background tasks
// ----------------------------------------------------------------------------
void loop() {
  // FreeRTOS tasks run concurrently in the background.
  vTaskDelay(pdMS_TO_TICKS(1000));
}
