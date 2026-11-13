// ============================================================
//  pin_config.cpp -- Dynamic GPIO Pin Configuration implementation
// Guide: Naglo-load at nagse-save ito ng GPIO assignments sa /pins.json.
// Kapag wala o hindi mabasa ang config, factory defaults ang ginagamit; isValidGpio()
// ang pang-check kung puwedeng gamitin ang pin sa napiling input/output role.
// ============================================================
#include "pin_config.h"
#include "config.h"
#include "storage.h"
#include <ArduinoJson.h>

static const char* PINS_FILE = "/pins.json";

// Default hardware pins
static PinConfig s_pins = {
    PIN_SENSOR_A, // sensorA
    PIN_SENSOR_B, // sensorB
    PIN_SENSOR_C, // sensorC
    PIN_SENSOR_D, // sensorD
    PIN_BTN_1, // btn1
    PIN_BTN_2, // btn2
    PIN_BTN_3, // btn3
    PIN_BTN_4, // btn4
    PIN_IR_1, // ir1
    PIN_IR_2, // ir2
    32, // ir3
    33, // ir4
    18, // servo1
    19, // servo2
    21, // servo3
    25, // servo4
    PIN_DOOR_SERVO, // doorServo
    23, // lcdSda
    22  // lcdScl
};

bool isValidGpio(int pin, bool allowInputOnly) {
    const int usable[] = {2, 4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33, 34, 35, 36, 39};
    bool found = false;
    for (int value : usable) if (value == pin) found = true;
    return found && (allowInputOnly || (pin != 34 && pin != 35 && pin != 36 && pin != 39));
}

static bool validConfig(const PinConfig& cfg) {
    const int inputOnlyAllowed[] = {cfg.sensorA, cfg.sensorB, cfg.sensorC, cfg.sensorD};
    const int outputCapable[] = {cfg.btn1, cfg.btn2, cfg.btn3, cfg.btn4,
        cfg.ir1, cfg.ir2, cfg.ir3, cfg.ir4, cfg.servo1, cfg.servo2,
        cfg.servo3, cfg.servo4, cfg.doorServo, cfg.lcdSda, cfg.lcdScl};
    int all[19];
    int count = 0;
    for (int pin : inputOnlyAllowed) {
        if (!isValidGpio(pin, true)) return false;
        all[count++] = pin;
    }
    for (int pin : outputCapable) {
        if (!isValidGpio(pin, false)) return false;
        all[count++] = pin;
    }
    for (int i = 0; i < count; ++i)
        for (int j = i + 1; j < count; ++j)
            if (all[i] == all[j]) return false;
    return true;
}

bool initPinConfig() {
    if (!fileExists(PINS_FILE)) {
        Serial.println("[PinConfig] No custom pins file found, using factory defaults.");
        return true;
    }

    String json = readFile(PINS_FILE);
    if (json.isEmpty()) return false;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        Serial.printf("[PinConfig] JSON error: %s, using defaults.\n", err.c_str());
        return false;
    }

    s_pins.sensorA = doc["sensorA"] | s_pins.sensorA;
    s_pins.sensorB = doc["sensorB"] | s_pins.sensorB;
    s_pins.sensorC = doc["sensorC"] | s_pins.sensorC;
    s_pins.sensorD = doc["sensorD"] | s_pins.sensorD;
    s_pins.btn1    = doc["btn1"]    | s_pins.btn1;
    s_pins.btn2    = doc["btn2"]    | s_pins.btn2;
    s_pins.btn3    = doc["btn3"]    | s_pins.btn3;
    s_pins.btn4    = doc["btn4"]    | s_pins.btn4;
    s_pins.ir1     = doc["ir1"]     | s_pins.ir1;
    s_pins.ir2     = doc["ir2"]     | s_pins.ir2;
    s_pins.ir3     = doc["ir3"]     | s_pins.ir3;
    s_pins.ir4     = doc["ir4"]     | s_pins.ir4;
    s_pins.servo1  = doc["servo1"]  | s_pins.servo1;
    s_pins.servo2  = doc["servo2"]  | s_pins.servo2;
    s_pins.servo3  = doc["servo3"]  | s_pins.servo3;
    s_pins.servo4  = doc["servo4"]  | s_pins.servo4;
    s_pins.doorServo = doc["doorServo"] | s_pins.doorServo;
    s_pins.lcdSda  = doc["lcdSda"]  | (doc["dispDio"] | s_pins.lcdSda);
    s_pins.lcdScl  = doc["lcdScl"]  | (doc["dispClk"] | s_pins.lcdScl);

    bool migratedPreviousDefaultPins = s_pins.sensorA == 34 && s_pins.sensorB == 35 &&
        ((s_pins.sensorC == 2 && s_pins.sensorD == 15) || (s_pins.sensorC == 36 && s_pins.sensorD == 39)) &&
        ((s_pins.btn1 == 13 && s_pins.btn2 == 14 && s_pins.btn3 == 27 && s_pins.btn4 == 26) ||
         (s_pins.btn1 == PIN_BTN_1 && s_pins.btn2 == PIN_BTN_2 &&
          s_pins.btn3 == PIN_BTN_3 && s_pins.btn4 == PIN_BTN_4)) &&
        ((s_pins.ir1 == 16 && s_pins.ir2 == 17) || (s_pins.ir1 == 36 && s_pins.ir2 == 39)) &&
        s_pins.ir3 == 32 && s_pins.ir4 == 33;
    bool migratedRecentDefaultPins = s_pins.sensorA == 13 && s_pins.sensorB == 14 &&
        s_pins.sensorC == 27 && s_pins.sensorD == 26 &&
        s_pins.btn1 == PIN_BTN_1 && s_pins.btn2 == PIN_BTN_2 &&
        s_pins.btn3 == PIN_BTN_3 && s_pins.btn4 == PIN_BTN_4 &&
        s_pins.ir1 == 16 && s_pins.ir2 == 17 && s_pins.ir3 == 32 && s_pins.ir4 == 33;
    if (migratedPreviousDefaultPins || migratedRecentDefaultPins) {
        s_pins.sensorA = PIN_SENSOR_A; s_pins.sensorB = PIN_SENSOR_B;
        s_pins.sensorC = PIN_SENSOR_C; s_pins.sensorD = PIN_SENSOR_D;
        s_pins.btn1 = PIN_BTN_1; s_pins.btn2 = PIN_BTN_2;
        s_pins.btn3 = PIN_BTN_3; s_pins.btn4 = PIN_BTN_4;
        s_pins.ir1 = PIN_IR_1; s_pins.ir2 = PIN_IR_2;
        s_pins.ir3 = PIN_IR_3; s_pins.ir4 = PIN_IR_4;
    }

    // Migrate older VP/VN IR assignments when they are custom, not a full default set.
    bool migratedIrPins = false;
    if (!migratedPreviousDefaultPins && !migratedRecentDefaultPins) {
        if (s_pins.ir1 == 36) { s_pins.ir1 = PIN_IR_1; migratedIrPins = true; }
        if (s_pins.ir2 == 39) { s_pins.ir2 = PIN_IR_2; migratedIrPins = true; }
    }

    bool migratedBottlePins = !migratedPreviousDefaultPins && s_pins.sensorC == 36 && s_pins.sensorD == 39;
    if (migratedBottlePins) { s_pins.sensorC = 2; s_pins.sensorD = 15; }

    if (!validConfig(s_pins)) {
        Serial.println("[PinConfig] Invalid or duplicate GPIO assignment.");
        return false;
    }
    if (migratedIrPins || migratedBottlePins || migratedPreviousDefaultPins || migratedRecentDefaultPins) {
        if (!savePinConfig(s_pins)) return false;
        Serial.println("[PinConfig] Migrated previous sensor pin assignments.");
    }

    Serial.println("[PinConfig] Custom GPIO pins loaded successfully from LittleFS.");
    return true;
}

const PinConfig& getPinConfig() {
    return s_pins;
}

bool savePinConfig(const PinConfig& cfg) {
    if (!validConfig(cfg)) return false;

    JsonDocument doc;
    doc["sensorA"] = cfg.sensorA;
    doc["sensorB"] = cfg.sensorB;
    doc["sensorC"] = cfg.sensorC;
    doc["sensorD"] = cfg.sensorD;
    doc["btn1"]    = cfg.btn1;
    doc["btn2"]    = cfg.btn2;
    doc["btn3"]    = cfg.btn3;
    doc["btn4"]    = cfg.btn4;
    doc["ir1"]     = cfg.ir1;
    doc["ir2"]     = cfg.ir2;
    doc["ir3"]     = cfg.ir3;
    doc["ir4"]     = cfg.ir4;
    doc["servo1"]  = cfg.servo1;
    doc["servo2"]  = cfg.servo2;
    doc["servo3"]  = cfg.servo3;
    doc["servo4"]  = cfg.servo4;
    doc["doorServo"] = cfg.doorServo;
    doc["lcdSda"]  = cfg.lcdSda;
    doc["lcdScl"]  = cfg.lcdScl;

    String out;
    serializeJson(doc, out);
    bool ok = writeFile(PINS_FILE, out);
    if (ok) s_pins = cfg;
    if (ok) {
        Serial.println("[PinConfig] Saved updated pins to LittleFS.");
    }
    return ok;
}

void resetPinConfigToDefaults() {
    PinConfig defaults = {PIN_SENSOR_A, PIN_SENSOR_B, PIN_SENSOR_C, PIN_SENSOR_D,
                          PIN_BTN_1, PIN_BTN_2, PIN_BTN_3, PIN_BTN_4, PIN_IR_1, PIN_IR_2, PIN_IR_3, PIN_IR_4,
                          18, 19, 21, 25, PIN_DOOR_SERVO, 23, 22};
    savePinConfig(defaults);
}
