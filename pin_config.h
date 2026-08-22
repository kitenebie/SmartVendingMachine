#pragma once
// ============================================================
//  pin_config.h -- Dynamic GPIO Pin Configuration for ESP32
//  Stores and loads customizable pin assignments from LittleFS
// ============================================================
#include <Arduino.h>

struct PinConfig {
    int sensorA;     // Bottle Sensor A
    int sensorB;     // Bottle Sensor B
    int btn1;        // Button Product 1
    int btn2;        // Button Product 2
    int btn3;        // Button Product 3
    int btn4;        // Button Product 4
    int ir1;         // IR Drop Product 1
    int ir2;         // IR Drop Product 2
    int ir3;         // IR Drop Product 3
    int ir4;         // IR Drop Product 4
    int servo1;      // Servo Motor 1
    int servo2;      // Servo Motor 2
    int servo3;      // Servo Motor 3
    int servo4;      // Servo Motor 4
    int lcdSda;      // 20x4 LCD I2C SDA
    int lcdScl;      // 20x4 LCD I2C SCL
};

/** Load pin configuration from LittleFS /pins.json (or defaults if missing). */
void initPinConfig();

/** Get current in-RAM pin configuration. */
const PinConfig& getPinConfig();

/** Save new pin configuration to LittleFS and update active pins. */
bool savePinConfig(const PinConfig& cfg);

/** Reset pins to factory defaults. */
void resetPinConfigToDefaults();

/** Validate whether a GPIO number is a safe, usable ESP32 pin. */
bool isValidGpio(int pin, bool allowInputOnly = true);
