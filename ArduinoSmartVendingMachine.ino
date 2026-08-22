// ============================================================
//  ESP32 Smart Vending Machine & Inventory Management System
//  Compatible with Arduino IDE & PlatformIO
//
//  Hardware: Dual Bottle Sensors + 4x Push Buttons + 4x Servos
//            + 4x IR Drop Sensors + 7-Segment Display (TM1637)
//            + Dynamic Web-Configurable GPIO Pins!
//  Web App: Standalone WiFi Hotspot + Dashboard + Stock CRUD
// ============================================================
#include <Arduino.h>
#include <ESPAsyncWebServer.h>

#include "config.h"
#include "pin_config.h"
#include "wifi_manager.h"
#include "storage.h"
#include "auth.h"
#include "products.h"
#include "display_manager.h"
#include "vending_controller.h"
#include "api.h"

// Web server on port 80
AsyncWebServer server(80);

// ---- Startup Banner ----------------------------------------
static void printBanner() {
    Serial.println();
    Serial.println("========================================");
    Serial.println("   ESP32 SMART VENDING MACHINE v1.0");
    Serial.println("   Mode: Hotspot + Web Inventory + Vending");
    Serial.println("========================================");
    Serial.println();
}

// ============================================================
void setup() {
    Serial.begin(115200);
    delay(500);
    printBanner();

    // 1. Mount LittleFS Filesystem
    Serial.println("[Init] Initializing LittleFS storage...");
    if (!initStorage()) {
        Serial.println("[FATAL] LittleFS mount failed! Halting.");
        while (true) delay(1000);
    }

    // 2. Load Dynamic GPIO Pin Configuration from /pins.json
    Serial.println("[Init] Loading GPIO Pin Configuration...");
    initPinConfig();

    // 3. Initialize default admin account in NVS
    Serial.println("[Init] Checking administrator account...");
    initDefaultCredentials();

    // 4. Load product database from /products.json
    Serial.println("[Init] Loading product inventory database...");
    if (!loadProducts()) {
        Serial.println("[WARN] Product load failed, starting with default list");
    }
    Serial.printf("[Init] Total products loaded: %d\n", (int)getProducts().size());

    // 5. Initialize Vending Machine Hardware (Dynamic Pins, Servos, Sensors, Display)
    Serial.println("[Init] Initializing Vending Machine hardware...");
    initVendingMachine();

    // 6. Start ESP32 WiFi Access Point (Hotspot)
    Serial.println("[Init] Starting WiFi Access Point...");
    if (!startAccessPoint(AP_SSID, AP_PASSWORD, AP_CHANNEL, false, AP_MAX_CONN)) {
        Serial.println("[WARN] Access point failed to start!");
    }

    // 7. Register API routes and start Async Web Server
    Serial.println("[Init] Starting Web Server...");
    setupAPI(server);
    server.begin();

    Serial.println();
    Serial.println("========================================");
    Serial.println(" >>> VENDING SYSTEM READY & ONLINE! <<<");
    Serial.println("----------------------------------------");
    Serial.printf(" Hotspot SSID    : %s\n", AP_SSID);
    if (strlen(AP_PASSWORD) > 0)
        Serial.printf(" Hotspot Pass    : %s\n", AP_PASSWORD);
    else
        Serial.println(" Hotspot Pass    : (OPEN NETWORK)");
    Serial.printf(" Web Dashboard   : http://%s\n", getLocalIP().c_str());
    Serial.println(" Default Login   : admin / admin123");
    Serial.println("========================================");
    Serial.println();
}

// ============================================================
void loop() {
    // Non-blocking state machine updates (Bottle detection, buttons, servos, IR sensors)
    updateVendingMachine();

    // Small delay to prevent task starvation
    delay(5);
}
