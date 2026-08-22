// ============================================================
//  storage.cpp — LittleFS helper implementation
// ============================================================
#include "storage.h"
#include <LittleFS.h>

bool initStorage() {
    if (!LittleFS.begin(false)) {                  // try mount without format
        Serial.println("[Storage] Mounting failed, formatting…");
        if (!LittleFS.begin(true)) {               // format + mount
            Serial.println("[Storage] ERROR: Could not mount LittleFS!");
            return false;
        }
    }
    Serial.println("[Storage] LittleFS mounted OK");
    return true;
}

String readFile(const char* path) {
    if (!LittleFS.exists(path)) return "";

    File f = LittleFS.open(path, "r");
    if (!f) {
        Serial.printf("[Storage] Failed to open %s for reading\n", path);
        return "";
    }
    String content = f.readString();
    f.close();
    return content;
}

bool writeFile(const char* path, const String& content) {
    File f = LittleFS.open(path, "w");
    if (!f) {
        Serial.printf("[Storage] Failed to open %s for writing\n", path);
        return false;
    }
    f.print(content);
    f.close();
    return true;
}

bool deleteFile(const char* path) {
    if (!LittleFS.exists(path)) return false;
    return LittleFS.remove(path);
}

bool fileExists(const char* path) {
    return LittleFS.exists(path);
}
