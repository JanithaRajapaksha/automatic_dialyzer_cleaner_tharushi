#include "web_server.h"

static WebServer server(WEB_SERVER_PORT);

static void handleRoot() {
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

  String response = "{\"success\":true,\"state\":" + String(state == 1 ? "true" : "false") + "}";
  server.send(200, "application/json", response);
}

static void handleStatus() {
  bool states[NUM_RELAYS];
  getAllRelayStates(states, NUM_RELAYS);

  String response = "[";
  for (int i = 0; i < NUM_RELAYS; i++) {
    if (i > 0) response += ",";
    response += states[i] ? "true" : "false";
  }
  response += "]";

  server.send(200, "application/json", response);
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

void initWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/relay", HTTP_GET, handleRelay);
  server.on("/all", HTTP_GET, handleAll);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/info", HTTP_GET, handleInfo);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("[WebServer] HTTP server started on port " + String(WEB_SERVER_PORT));
}

void handleWebServerClient() {
  server.handleClient();
}

