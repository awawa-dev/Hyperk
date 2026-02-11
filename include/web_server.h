// File: include/web_server.h

#pragma once
#include <ESPAsyncWebServer.h>

void setupWebServer(AsyncWebServer& server);
String templateProcessor(const String& var);