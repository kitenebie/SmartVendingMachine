// ============================================================
//  storage.cpp — LittleFS helper implementation
// Guide: Iisang lugar ito para i-mount ang LittleFS at magbasa,
// magsulat, magtanggal o mag-check ng files. Boolean ang balik ng write/delete
// operations para malaman ng tumatawag kung nagtagumpay.
// ============================================================
#include "storage.h"
#include <LittleFS.h>

bool initStorage() {
    if (!LittleFS.begin(false)) {
        Serial.println("[Storage] ERROR: Could not mount LittleFS; refusing to erase stored accounts.");
        return false;
    }
    Serial.println("[Storage] LittleFS mounted OK");
    if (!LittleFS.exists("/data")) LittleFS.mkdir("/data");
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
    String temporary = String(path) + ".tmp";
    File f = LittleFS.open(temporary, "w");
    if (!f) {
        Serial.printf("[Storage] Failed to open %s for writing\n", path);
        return false;
    }
    size_t written = f.print(content);
    f.close();
    if (written != content.length()) {
        LittleFS.remove(temporary);
        Serial.printf("[Storage] Incomplete write to %s\n", path);
        return false;
    }
    if (!LittleFS.rename(temporary, path)) {
        LittleFS.remove(temporary);
        Serial.printf("[Storage] Failed to replace %s\n", path);
        return false;
    }
    return true;
}

bool deleteFile(const char* path) {
    if (!LittleFS.exists(path)) return false;
    return LittleFS.remove(path);
}

bool fileExists(const char* path) {
    return LittleFS.exists(path);
}
