#pragma once
// ============================================================
//  config.h -- ESP32 Smart Vending Machine & Hotspot Settings
//  Updated for 20x4 I2C LCD Display (PCF8574 Adapter)
// ============================================================
#include <Arduino.h>

// ---- WiFi Access Point (Hotspot) Settings ------------------
#define AP_SSID         "ESP32-Inventory"
#define AP_PASSWORD     "inventory123"    // Min. 8 characters (or "" for open network)
#define AP_CHANNEL      1
#define AP_MAX_CONN     4

// ---- Bottle Detection Proximity Sensors -------------------
#define PIN_SENSOR_A    34     // Upper proximity sensor (input only)
#define PIN_SENSOR_B    35     // Lower proximity sensor (input only)
#define BOTTLE_VALIDATION_TIME_MS  200   // Both sensors must be active for this duration
#define BOTTLE_DEBOUNCE_MS         150

// ---- Product Selection Buttons (4 Buttons) -----------------
#define PIN_BTN_1       13     // Product 1 push button (internal pull-up)
#define PIN_BTN_2       14     // Product 2 push button (internal pull-up)
#define PIN_BTN_3       27     // Product 3 push button (internal pull-up)
#define PIN_BTN_4       26     // Product 4 push button (internal pull-up)
#define BTN_DEBOUNCE_MS 50

// ---- IR Drop / Delivery Sensors (4 Sensors) ----------------
#define PIN_IR_1        36     // IR sensor 1 (VP / input only)
#define PIN_IR_2        39     // IR sensor 2 (VN / input only)
#define PIN_IR_3        32     // IR sensor 3 (input)
#define PIN_IR_4        33     // IR sensor 4 (input)

// ---- Dispensing Servos (4 Servos) --------------------------
#define PIN_SERVO_1     18     // Servo motor 1
#define PIN_SERVO_2     19     // Servo motor 2
#define PIN_SERVO_3     21     // Servo motor 3
#define PIN_SERVO_4     25     // Servo motor 4
#define DISPENSE_TIMEOUT_MS  5000  // Maximum run time before auto-stopping stuck servo

// ---- 20x4 I2C LCD Display (PCF8574) -----------------------
#define PIN_LCD_SDA     23     // I2C Data (SDA)
#define PIN_LCD_SCL     22     // I2C Clock (SCL)
#define LCD_I2C_ADDR    0x27   // Common PCF8574 address: 0x27 or 0x3F
#define LCD_COLS        20
#define LCD_ROWS        4
