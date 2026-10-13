// ============================================================
//  wifi_manager.cpp -- ESP32 Access Point (Hotspot) mode
// Guide: startAccessPoint() ang nagse-set ng ESP32 sa hotspot mode
// gamit ang SSID at network settings mula sa tumatawag. getLocalIP() ang
// nagbibigay ng address na bubuksan para ma-access ang web dashboard.
// ============================================================
#include "wifi_manager.h"
#include <WiFi.h>
#include <Preferences.h>
#include <esp_random.h>
#include <time.h>

bool loadAccessPointPassword(String& password) {
    Preferences prefs;
    if (!prefs.begin("vending-net", false)) return false;
    password = prefs.getString("password", "");
    if (password.length() < 8) {
        char generated[17];
        snprintf(generated, sizeof(generated), "%08lx%08lx",
                 (unsigned long)esp_random(), (unsigned long)esp_random());
        password = generated;
        if (prefs.putString("password", password) == 0) {
            prefs.end();
            return false;
        }
    }
    prefs.end();
    return true;
}

bool loadStationCredentials(String& ssid, String& password) {
    Preferences prefs;
    if (!prefs.begin("vending-sta", true)) return false;
    ssid = prefs.getString("ssid", ""); password = prefs.getString("password", ""); prefs.end();
    return true;
}

bool saveStationCredentials(const String& ssid, const String& password) {
    if (ssid.isEmpty() || ssid.length() > 32 || password.length() > 63) return false;
    Preferences prefs;
    if (!prefs.begin("vending-sta", false)) return false;
    bool saved = prefs.putString("ssid", ssid) == ssid.length() && prefs.putString("password", password) == password.length();
    prefs.end();
    return saved;
}

void connectToStation(const String& ssid, const String& password) {
    if (ssid.isEmpty()) return;
    WiFi.begin(ssid.c_str(), password.c_str());
    configTzTime("PST-8", "pool.ntp.org", "time.nist.gov");
    Serial.printf("[WiFi] Connecting to configured network: %s\n", ssid.c_str());
}

bool stationConnected() { return WiFi.status() == WL_CONNECTED; }
bool timeSynchronized() { return time(nullptr) >= 1700000000; }

bool startAccessPoint(const char* ssid,
                      const char* password,
                      int         channel,
                      bool        hidden,
                      int         maxConn) {

    Serial.println("[WiFi] Starting Access Point (hotspot) mode...");

    // Keep the hotspot available while station mode connects for NTP.
    WiFi.mode(WIFI_AP_STA);

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
