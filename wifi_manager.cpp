// ============================================================
//  wifi_manager.cpp -- ESP32 Access Point (Hotspot) mode
// ============================================================
#include "wifi_manager.h"
#include <WiFi.h>

bool startAccessPoint(const char* ssid,
                      const char* password,
                      int         channel,
                      bool        hidden,
                      int         maxConn) {

    Serial.println("[WiFi] Starting Access Point (hotspot) mode...");

    // Switch to AP-only mode (not STA)
    WiFi.mode(WIFI_AP);

    // password == "" means open/unsecured network
    bool ok;
    if (strlen(password) == 0) {
        ok = WiFi.softAP(ssid, nullptr, channel, hidden, maxConn);
        Serial.println("[WiFi] Network is OPEN (no password)");
    } else {
        ok = WiFi.softAP(ssid, password, channel, hidden, maxConn);
    }

    if (!ok) {
        Serial.println("[WiFi] ERROR: Failed to start Access Point!");
        return false;
    }

    delay(100); // give the AP a moment to initialise

    IPAddress ip = WiFi.softAPIP();
    Serial.println("[WiFi] Access Point started!");
    Serial.printf("[WiFi] SSID    : %s\n", ssid);
    if (strlen(password) > 0)
        Serial.printf("[WiFi] Password: %s\n", password);
    Serial.printf("[WiFi] IP Addr : %s\n", ip.toString().c_str());
    Serial.printf("[WiFi] Open http://%s in your browser\n", ip.toString().c_str());

    return true;
}

String getLocalIP() {
    return WiFi.softAPIP().toString();
}
