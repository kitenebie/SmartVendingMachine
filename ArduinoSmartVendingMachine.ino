// ============================================================
//  ESP32 Smart Vending Machine & Inventory Management System
//  Compatible with Arduino IDE & PlatformIO
//
//  Hardware: Dual Bottle Sensors + 4x Push Buttons + 4x Servos
//            + 4x IR Drop Sensors + 20x4 I2C LCD (PCF8574)
//            + Dynamic Web-Configurable GPIO Pins!
//  Web App: Standalone WiFi Hotspot + Dashboard + Stock CRUD
// Guide: Main entry point ito ng buong machine. Sa setup(), ini-initialize
// ang storage, users, products at hardware bago buksan ang WiFi hotspot at API;
// sa loop(), tuloy-tuloy na ina-update ang vending at bottle collection.
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
    if (!initPinConfig()) {
        Serial.println("[FATAL] Invalid GPIO configuration. Halting.");
        while (true) delay(1000);
    }

    // 3. Load user accounts from LittleFS
    Serial.println("[Init] Loading user accounts...");
    if (!initUsers()) {
        Serial.println("[FATAL] User data could not be loaded or saved. Halting.");
        while (true) delay(1000);
    }

    // 4. Load product database from /products.json
    Serial.println("[Init] Loading product inventory database...");
    if (!loadProducts() || !recoverRedemption()) {
        Serial.println("[FATAL] Product data or pending redemption could not be recovered. Halting.");
        while (true) delay(1000);
    }
    Serial.printf("[Init] Total products loaded: %d\n", (int)getProducts().size());

    // 5. Initialize Vending Machine Hardware (Dynamic Pins, Servos, Sensors, Display)
    Serial.println("[Init] Initializing Vending Machine hardware...");
    if (!initVendingMachine()) {
        Serial.println("[FATAL] Hardware initialization failed. Halting.");
        while (true) delay(1000);
    }

    // 6. Start ESP32 WiFi Access Point (Hotspot)
    Serial.println("[Init] Starting WiFi Access Point...");
    String accessPointPassword;
    if (!loadAccessPointPassword(accessPointPassword) ||
        !startAccessPoint(AP_SSID, accessPointPassword.c_str(), AP_CHANNEL, false, AP_MAX_CONN)) {
        Serial.println("[FATAL] Access point could not start. Halting.");
        while (true) delay(1000);
    }

    // 7. Register API routes and start Async Web Server
    Serial.println("[Init] Starting Web Server...");
    if (!setupAPI(server)) {
        Serial.println("[FATAL] Web API initialization failed. Halting.");
        while (true) delay(1000);
    }
    server.begin();

    Serial.println();
    Serial.println("========================================");
    Serial.println(" >>> VENDING SYSTEM READY & ONLINE! <<<");
    Serial.println("----------------------------------------");
    Serial.printf(" Hotspot SSID    : %s\n", AP_SSID);
    Serial.printf(" Hotspot Pass    : %s\n", accessPointPassword.c_str());
    Serial.printf(" Web Dashboard   : http://%s\n", getLocalIP().c_str());
    Serial.println(" Admin username  : admin@local");
    Serial.println("========================================");
    Serial.println();
}

// ============================================================
void loop() {
    // Non-blocking state machine updates (Bottle detection, buttons, servos, IR sensors)
    updateVendingMachine();
    updateReverseVending();

    // Small delay to prevent task starvation
    delay(5);
}
