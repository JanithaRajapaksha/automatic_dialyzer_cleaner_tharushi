#include "web_server.h"
#include "reprocessor.h"
#include "pressure_sensor.h"
#include <WebSocketsServer.h>

static WebServer server(WEB_SERVER_PORT);
static WebSocketsServer wsServer(WEBSOCKET_PORT);
static volatile bool s_needBroadcast = false;
static volatile bool s_needReprocessorBroadcast = false;

static String getStatusJson() {
  bool states[NUM_RELAYS] = {false};
  getAllRelayStates(states, NUM_RELAYS);

  String response = "{\"type\":\"status\",\"states\":[";
  for (int i = 0; i < NUM_RELAYS; i++) {
    if (i > 0) response += ",";
    response += states[i] ? "true" : "false";
  }
  response += "]";
  // Indexed keys for full backward compatibility with older cached browser JS
  for (int i = 0; i < NUM_RELAYS; i++) {
    response += ",\"" + String(i) + "\":" + (states[i] ? "true" : "false");
  }
  response += "}";
  return response;
}

void broadcastRelayStatus() {
  String json = getStatusJson();
  wsServer.broadcastTXT(json);
}

void broadcastReprocessorStatus() {
  s_needReprocessorBroadcast = true;
  s_needBroadcast = true;
}

static void handleWebSocketMessage(uint8_t num, const String& msg) {
  // 1. Reprocessor Cycle Commands
  if (msg == "START" || msg == "CMD:START" || msg == "AUTO") {
    startAutoCycle();
  } else if (msg == "PAUSE" || msg == "CMD:PAUSE") {
    pauseCycle();
  } else if (msg == "RESUME" || msg == "CMD:RESUME") {
    resumeCycle();
  } else if (msg == "ABORT" || msg == "CMD:ABORT" || msg == "STOP") {
    abortCycle("WebSocket Request");
  } else if (msg == "ESTOP" || msg == "CMD:ESTOP") {
    emergencyStop();
  } else if (msg.startsWith("STAGE:") || msg.startsWith("CMD:STAGE:")) {
    int colonIdx = msg.lastIndexOf(':');
    int stageId = msg.substring(colonIdx + 1).toInt();
    startSingleStage(stageId);
  }
  // 2. High-speed compact manual command: SET:<ch>:<state> (e.g., SET:0:1)
  else if (msg.startsWith("SET:")) {
    int firstColon = msg.indexOf(':');
    int secondColon = msg.indexOf(':', firstColon + 1);
    if (secondColon != -1) {
      int ch = msg.substring(firstColon + 1, secondColon).toInt();
      int state = msg.substring(secondColon + 1).toInt();
      if (ch >= 0 && ch < NUM_RELAYS && (state == 0 || state == 1)) {
        setRelay(ch, state == 1);
        s_needBroadcast = true;
      }
    }
  } else if (msg.startsWith("ALL:")) {
    int state = msg.substring(4).toInt();
    if (state == 0 || state == 1) {
      setAllRelays(state == 1);
      s_needBroadcast = true;
    }
  } else if (msg.startsWith("STATUS")) {
    String stRelays = getStatusJson();
    String stRep = getReprocessorStatusJson();
    String stPress = getPressureJson();
    wsServer.sendTXT(num, stRelays);
    wsServer.sendTXT(num, stRep);
    wsServer.sendTXT(num, stPress);
  } else if (msg == "TARE" || msg == "CMD:TARE") {
    tarePressureSensor(8);
    String stPress = getPressureJson();
    wsServer.sendTXT(num, stPress);
  } else if (msg == "PRESSURE") {
    String stPress = getPressureJson();
    wsServer.sendTXT(num, stPress);
  }
  // 3. JSON Fallbacks
  else if (msg.indexOf("\"set\"") != -1) {
    int chIdx = msg.indexOf("\"ch\":");
    int stateIdx = msg.indexOf("\"state\":");
    if (chIdx != -1 && stateIdx != -1) {
      int ch = msg.substring(chIdx + 5).toInt();
      int state = msg.substring(stateIdx + 8).toInt();
      if (ch >= 0 && ch < NUM_RELAYS && (state == 0 || state == 1)) {
        setRelay(ch, state == 1);
        s_needBroadcast = true;
      }
    }
  } else if (msg.indexOf("\"all\"") != -1) {
    int stateIdx = msg.indexOf("\"state\":");
    if (stateIdx != -1) {
      int state = msg.substring(stateIdx + 8).toInt();
      if (state == 0 || state == 1) {
        setAllRelays(state == 1);
        s_needBroadcast = true;
      }
    }
  } else if (msg.indexOf("\"status\"") != -1) {
    String stRelays = getStatusJson();
    String stRep = getReprocessorStatusJson();
    wsServer.sendTXT(num, stRelays);
    wsServer.sendTXT(num, stRep);
  }
}

static void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[WS] Client [%u] Disconnected\n", num);
      break;

    case WStype_CONNECTED: {
      IPAddress ip = wsServer.remoteIP(num);
      Serial.printf("[WS] Client [%u] Connected from %s\n", num, ip.toString().c_str());
      String stRelays = getStatusJson();
      String stRep = getReprocessorStatusJson();
      String stPress = getPressureJson();
      wsServer.sendTXT(num, stRelays);
      wsServer.sendTXT(num, stRep);
      wsServer.sendTXT(num, stPress);
      break;
    }

    case WStype_TEXT: {
      if (length == 0 || payload == nullptr) break;
      char* safeBuf = (char*)malloc(length + 1);
      if (safeBuf) {
        memcpy(safeBuf, payload, length);
        safeBuf[length] = '\0';
        String msg = String(safeBuf);
        free(safeBuf);
        handleWebSocketMessage(num, msg);
      }
      break;
    }

    default:
      break;
  }
}

static void handleRoot() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "0");
  server.send_P(200, "text/html", webpage);
}

static void handleRelay() {
  if (!server.hasArg("ch") || !server.hasArg("state")) {
    server.send(400, "application/json", "{\"error\":\"Missing required arguments (ch, state)\"}");
    return;
  }

  int ch = server.arg("ch").toInt();
  int state = server.arg("state").toInt();

  if (ch < 0 || ch >= NUM_RELAYS || (state != 0 && state != 1)) {
    server.send(400, "application/json", "{\"error\":\"Invalid channel or state value\"}");
    return;
  }

  bool success = setRelay(ch, state == 1);
  if (!success) {
    server.send(500, "application/json", "{\"error\":\"Failed to update relay\"}");
    return;
  }

  // Trigger non-reentrant broadcast to all WebSocket clients
  s_needBroadcast = true;

  String response = "{";
  response += "\"channel\":" + String(ch + 1) + ",";
  response += "\"name\":\"" + String(getRelayName(ch)) + "\",";
  response += "\"state\":" + String(getRelayState(ch) ? "true" : "false");
  response += "}";

  server.send(200, "application/json", response);
}

static void handleAll() {
  if (!server.hasArg("state")) {
    server.send(400, "application/json", "{\"error\":\"Missing state argument\"}");
    return;
  }

  int state = server.arg("state").toInt();
  if (state != 0 && state != 1) {
    server.send(400, "application/json", "{\"error\":\"Invalid state value\"}");
    return;
  }

  setAllRelays(state == 1);

  // Trigger non-reentrant broadcast to all WebSocket clients
  s_needBroadcast = true;

  String response = "{\"success\":true,\"state\":" + String(state == 1 ? "true" : "false") + "}";
  server.send(200, "application/json", response);
}

static void handleStatus() {
  server.send(200, "application/json", getStatusJson());
}

static void handleInfo() {
  String response = "{";
  response += "\"system\":\"" + String(SYSTEM_NAME) + "\",";
  response += "\"version\":\"" + String(SYSTEM_VERSION) + "\",";
  response += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
  response += "\"uptimeSeconds\":" + String(millis() / 1000);
  response += "}";

  server.send(200, "application/json", response);
}

static void handleNotFound() {
  server.send(404, "application/json", "{\"error\":\"Endpoint not found\"}");
}

static void handleReprocessorStart() {
  startAutoCycle();
  server.send(200, "application/json", getReprocessorStatusJson());
}

static void handleReprocessorStage() {
  if (!server.hasArg("id")) {
    server.send(400, "application/json", "{\"error\":\"Missing stage id\"}");
    return;
  }
  int id = server.arg("id").toInt();
  if (id < 1 || id > 7) {
    server.send(400, "application/json", "{\"error\":\"Invalid stage id (1 to 7)\"}");
    return;
  }
  startSingleStage(id);
  server.send(200, "application/json", getReprocessorStatusJson());
}

static void handleReprocessorPause() {
  pauseCycle();
  server.send(200, "application/json", getReprocessorStatusJson());
}

static void handleReprocessorResume() {
  resumeCycle();
  server.send(200, "application/json", getReprocessorStatusJson());
}

static void handleReprocessorAbort() {
  abortCycle("Web Interface Request");
  server.send(200, "application/json", getReprocessorStatusJson());
}

static void handleReprocessorEstop() {
  emergencyStop();
  server.send(200, "application/json", "{\"estop\":true,\"message\":\"Emergency Stop Activated\"}");
}

static void handleReprocessorStatus() {
  server.send(200, "application/json", getReprocessorStatusJson());
}

static void handlePressure() {
  server.send(200, "application/json", getPressureJson());
}

static void handlePressureTare() {
  tarePressureSensor(8);
  server.send(200, "application/json", getPressureJson());
}

static void handlePressureScale() {
  if (server.hasArg("val")) {
    setPressureScale(server.arg("val").toFloat());
  }
  server.send(200, "application/json", getPressureJson());
}

void initWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/relay", HTTP_GET, handleRelay);
  server.on("/all", HTTP_GET, handleAll);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/info", HTTP_GET, handleInfo);

  // HX710B Air Pressure Sensor endpoints
  server.on("/pressure", HTTP_GET, handlePressure);
  server.on("/pressure/tare", HTTP_GET, handlePressureTare);
  server.on("/pressure/scale", HTTP_GET, handlePressureScale);

  // Reprocessor Sequential State Machine endpoints
  server.on("/reprocessor/start", HTTP_GET, handleReprocessorStart);
  server.on("/reprocessor/stage", HTTP_GET, handleReprocessorStage);
  server.on("/reprocessor/pause", HTTP_GET, handleReprocessorPause);
  server.on("/reprocessor/resume", HTTP_GET, handleReprocessorResume);
  server.on("/reprocessor/abort", HTTP_GET, handleReprocessorAbort);
  server.on("/reprocessor/estop", HTTP_GET, handleReprocessorEstop);
  server.on("/reprocessor/status", HTTP_GET, handleReprocessorStatus);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("[WebServer] HTTP server started on port " + String(WEB_SERVER_PORT));

  wsServer.begin();
  wsServer.onEvent(onWebSocketEvent);
  Serial.println("[WebSocket] Server started on port " + String(WEBSOCKET_PORT));
}

void handleWebServerClient() {
  server.handleClient();
  wsServer.loop();

  // Defer broadcast so it runs outside of wsServer.loop() and HTTP handleClient()
  if (s_needBroadcast) {
    s_needBroadcast = false;
    broadcastRelayStatus();
  }

  if (s_needReprocessorBroadcast) {
    s_needReprocessorBroadcast = false;
    String repJson = getReprocessorStatusJson();
    wsServer.broadcastTXT(repJson);
  }

  // Periodic pressure broadcast (every 100ms = 10 Hz real-time stream) only when clients are connected
  static unsigned long s_lastPressureBroadcast = 0;
  if (millis() - s_lastPressureBroadcast >= 100) {
    s_lastPressureBroadcast = millis();
    if (wsServer.connectedClients() > 0) {
      String pJson = getPressureJson();
      wsServer.broadcastTXT(pJson);
    }
  }
}
