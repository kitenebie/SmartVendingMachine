#pragma once
// ============================================================
//  wifi_manager.h -- WiFi hotspot and station mode
//
//  The ESP32 keeps its own WiFi network while optionally joining a router.
// ============================================================
#include <Arduino.h>

/** Read or create a unique per-device WiFi password in NVS. */
bool loadAccessPointPassword(String& password);
bool loadStationCredentials(String& ssid, String& password);
bool saveStationCredentials(const String& ssid, const String& password);
void connectToStation(const String& ssid, const String& password);
bool stationConnected();
bool timeSynchronized();
bool setSystemTime(uint32_t epochSeconds);

/**
 * Start the ESP32 as a WiFi Access Point while keeping station mode available.
 *
 * @param ssid     Network name broadcast by the ESP32
 * @param password Password (leave empty "" for open/unsecured network)
 * @param channel  WiFi channel (1-13, default 1)
 * @param hidden   Set true to hide the SSID broadcast
 * @param maxConn  Maximum simultaneous clients (1-4, ESP32 limit)
 *
 * Returns true on success.
 * Default AP IP address: 192.168.4.1
 */
bool startAccessPoint(const char* ssid,
                      const char* password = "",
                      int         channel  = 1,
                      bool        hidden   = false,
                      int         maxConn  = 4);

/** Return the ESP32 AP IP address as a String (always 192.168.4.1). */
String getLocalIP();
