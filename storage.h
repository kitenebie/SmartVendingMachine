#pragma once
// ============================================================
//  storage.h — LittleFS helper declarations
// ============================================================
#include <Arduino.h>

/**
 * Mount LittleFS.  If mounting fails the partition is
 * formatted automatically and re-mounted.
 * Returns true on success.
 */
bool initStorage();

/** Read entire file contents into a String. Returns "" on error. */
String readFile(const char* path);

/**
 * Write content to a file (overwrites).
 * Returns true on success.
 */
bool writeFile(const char* path, const String& content);

/** Delete a file. Returns true if the file was removed. */
bool deleteFile(const char* path);

/** Returns true if the file exists. */
bool fileExists(const char* path);
