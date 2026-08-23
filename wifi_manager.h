#pragma once
// ============================================================
//  wifi_manager.h -- WiFi Access Point (Hotspot) mode
//
//  The ESP32 creates its own WiFi network.
//  Devices connect directly to the ESP32 -- no router needed.
// ============================================================
#include <Arduino.h>

/**
 * Start the ESP32 as a WiFi Access Point (hotspot).
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
