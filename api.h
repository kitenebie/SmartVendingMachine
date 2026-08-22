#pragma once
// ============================================================
//  api.h — REST API and static file server declarations
// ============================================================
#include <ESPAsyncWebServer.h>

/**
 * Register all REST endpoints and the static-file catch-all
 * on the provided AsyncWebServer instance.
 */
void setupAPI(AsyncWebServer& server);

/**
 * Extract the session token from an incoming request.
 * Checks the "X-Session-Token" header first, then the
 * "token" query parameter.
 * Returns an empty string if not present.
 */
String extractToken(AsyncWebServerRequest* request);

/**
 * Helper used by route handlers: returns true if the request
 * carries a valid session token.
 */
bool isAuthenticated(AsyncWebServerRequest* request);
