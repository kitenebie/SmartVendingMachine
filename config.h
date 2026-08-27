#pragma once
// ============================================================
//  config.h -- ESP32 Smart Vending Machine & Hotspot Settings
//  Updated for 20x4 I2C LCD Display (PCF8574 Adapter)
// ============================================================
#include <Arduino.h>

// ESP32 38-pin board silk-screen guide:
// Pxx means GPIO xx (for example, P23 = GPIO 23).
// This project uses normal P-labelled pins for every IR sensor; VP/VN are not used.
// Do not use SD0, SD1, SD2, SD3, CMD, or CLK: they are connected to the ESP32 flash.

// ---- WiFi Access Point (Hotspot) Settings ------------------
#define AP_SSID         "ESP32-Inventory"
#define AP_PASSWORD     "inventory123"    // Min. 8 characters (or "" for open network)
#define AP_CHANNEL      1
#define AP_MAX_CONN     4

// ---- Bottle Detection Proximity Sensors -------------------
#define PIN_SENSOR_A    34     // Upper proximity sensor — board pin P34 / GPIO 34 (input only)
#define PIN_SENSOR_B    35     // Lower proximity sensor — board pin P35 / GPIO 35 (input only)
#define BOTTLE_VALIDATION_TIME_MS  200   // Both sensors must be active for this duration
#define BOTTLE_DEBOUNCE_MS         150

// ---- Product Selection Buttons (4 Buttons) -----------------
#define PIN_BTN_1       13     // Product 1 button — board pin P13 / GPIO 13 (internal pull-up)
#define PIN_BTN_2       14     // Product 2 button — board pin P14 / GPIO 14 (internal pull-up)
#define PIN_BTN_3       27     // Product 3 button — board pin P27 / GPIO 27 (internal pull-up)
#define PIN_BTN_4       26     // Product 4 button — board pin P26 / GPIO 26 (internal pull-up)
#define BTN_DEBOUNCE_MS 50

// ---- IR Drop / Delivery Sensors (4 Sensors) ----------------
#define PIN_IR_1        16     // IR sensor 1 — board pin P16 / GPIO 16 (input)
#define PIN_IR_2        17     // IR sensor 2 — board pin P17 / GPIO 17 (input)
#define PIN_IR_3        32     // IR sensor 3 — board pin P32 / GPIO 32 (input)
#define PIN_IR_4        33     // IR sensor 4 — board pin P33 / GPIO 33 (input)

// ---- Dispensing Servos (4 Servos) --------------------------
#define PIN_SERVO_1     18     // Servo motor 1 — board pin P18 / GPIO 18
#define PIN_SERVO_2     19     // Servo motor 2 — board pin P19 / GPIO 19
#define PIN_SERVO_3     21     // Servo motor 3 — board pin P21 / GPIO 21
#define PIN_SERVO_4     25     // Servo motor 4 — board pin P25 / GPIO 25
#define DISPENSE_TIMEOUT_MS  5000  // Maximum run time before auto-stopping stuck servo

// ---- 20x4 I2C LCD Display (PCF8574) -----------------------
#define PIN_LCD_SDA     23     // I2C Data (SDA) — board pin P23 / GPIO 23
#define PIN_LCD_SCL     22     // I2C Clock (SCL) — board pin P22 / GPIO 22
#define LCD_I2C_ADDR    0x27   // Common PCF8574 address: 0x27 or 0x3F
#define LCD_COLS        16
#define LCD_ROWS        2
