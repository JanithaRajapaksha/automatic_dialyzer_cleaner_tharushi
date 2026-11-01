#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WebServer.h>
#include "config.h"
#include "relays.h"
#include "webpage.h"

// Initialize web server routes and start listening
void initWebServer();

// Handle client requests (called inside the FreeRTOS WebServer task)
void handleWebServerClient();

#endif // WEB_SERVER_H

