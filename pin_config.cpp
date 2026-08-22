// ============================================================
//  pin_config.cpp -- Dynamic GPIO Pin Configuration implementation
// ============================================================
#include "pin_config.h"
#include "storage.h"
#include <ArduinoJson.h>

static const char* PINS_FILE = "/pins.json";

// Default hardware pins
static PinConfig s_pins = {
    34, // sensorA
    35, // sensorB
    13, // btn1
    14, // btn2
    27, // btn3
    26, // btn4
    16, // ir1 (P16)
    17, // ir2 (P17)
    32, // ir3
    33, // ir4
    18, // servo1
    19, // servo2
    21, // servo3
    25, // servo4
    23, // lcdSda
    22  // lcdScl
};

bool isValidGpio(int pin, bool allowInputOnly) {
    if (pin < 0 || pin > 39) return false;
    // Reserved for SPI Flash: 6, 7, 8, 9, 10, 11
    if (pin >= 6 && pin <= 11) return false;
    // Input-only pins: 34, 35, 36, 39
    if (!allowInputOnly && (pin == 34 || pin == 35 || pin == 36 || pin == 39)) {
        return false;
    }
    return true;
}

void initPinConfig() {
    if (!fileExists(PINS_FILE)) {
        Serial.println("[PinConfig] No custom pins file found, using factory defaults.");
        savePinConfig(s_pins);
        return;
    }

    String json = readFile(PINS_FILE);
    if (json.isEmpty()) return;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        Serial.printf("[PinConfig] JSON error: %s, using defaults.\n", err.c_str());
        return;
    }

    s_pins.sensorA = doc["sensorA"] | s_pins.sensorA;
    s_pins.sensorB = doc["sensorB"] | s_pins.sensorB;
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
    s_pins.lcdSda  = doc["lcdSda"]  | (doc["dispDio"] | s_pins.lcdSda);
    s_pins.lcdScl  = doc["lcdScl"]  | (doc["dispClk"] | s_pins.lcdScl);

    // Migrate the previous IR 1/IR 2 defaults away from VP/VN to P16/P17.
    bool migratedIrPins = false;
    if (s_pins.ir1 == 36) { s_pins.ir1 = 16; migratedIrPins = true; }
    if (s_pins.ir2 == 39) { s_pins.ir2 = 17; migratedIrPins = true; }

    if (migratedIrPins) {
        savePinConfig(s_pins);
        Serial.println("[PinConfig] Migrated IR 1/IR 2 from VP/VN to P16/P17.");
    }

    Serial.println("[PinConfig] Custom GPIO pins loaded successfully from LittleFS.");
}

const PinConfig& getPinConfig() {
    return s_pins;
}

bool savePinConfig(const PinConfig& cfg) {
    s_pins = cfg;

    JsonDocument doc;
    doc["sensorA"] = s_pins.sensorA;
    doc["sensorB"] = s_pins.sensorB;
    doc["btn1"]    = s_pins.btn1;
    doc["btn2"]    = s_pins.btn2;
    doc["btn3"]    = s_pins.btn3;
    doc["btn4"]    = s_pins.btn4;
    doc["ir1"]     = s_pins.ir1;
    doc["ir2"]     = s_pins.ir2;
    doc["ir3"]     = s_pins.ir3;
    doc["ir4"]     = s_pins.ir4;
    doc["servo1"]  = s_pins.servo1;
    doc["servo2"]  = s_pins.servo2;
    doc["servo3"]  = s_pins.servo3;
    doc["servo4"]  = s_pins.servo4;
    doc["lcdSda"]  = s_pins.lcdSda;
    doc["lcdScl"]  = s_pins.lcdScl;

    String out;
    serializeJson(doc, out);
    bool ok = writeFile(PINS_FILE, out);
    if (ok) {
        Serial.println("[PinConfig] Saved updated pins to LittleFS.");
    }
    return ok;
}

void resetPinConfigToDefaults() {
    PinConfig defaults = {34, 35, 13, 14, 27, 26, 16, 17, 32, 33, 18, 19, 21, 25, 23, 22};
    savePinConfig(defaults);
}
