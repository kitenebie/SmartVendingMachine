#pragma once
// ============================================================
//  storage.h — LittleFS helper declarations
// ============================================================
#include <Arduino.h>

/**
 * Mount LittleFS without formatting existing data on failure.
 * Returns true on success.
 */
bool initStorage();

/** Read entire file contents into a String. Returns "" on error. */
String readFile(const char* path);

/**
 * Write content to a temporary file and replace the target after a full write.
 * Returns true on success.
 */
bool writeFile(const char* path, const String& content);

/** Delete a file. Returns true if the file was removed. */
bool deleteFile(const char* path);

/** Returns true if the file exists. */
bool fileExists(const char* path);
