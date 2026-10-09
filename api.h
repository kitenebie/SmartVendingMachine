#pragma once
#include <ESPAsyncWebServer.h>
bool setupAPI(AsyncWebServer& server);
bool recoverRedemption();
void updateReverseVending();
