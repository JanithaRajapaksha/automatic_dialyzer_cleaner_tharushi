#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WebServer.h>
#include "config.h"
#include "relays.h"
#include "webpage.h"

// Initialize web server routes, WebSocket server, and start listening
void initWebServer();

// Handle client requests and WebSocket events (called inside the FreeRTOS WebServer task)
void handleWebServerClient();

// Broadcast current relay status to all connected WebSocket clients
void broadcastRelayStatus();

// Broadcast current reprocessor FSM status to all connected WebSocket clients
void broadcastReprocessorStatus();

#endif // WEB_SERVER_H

