#pragma once
// ============================================================
//  auth.h — Credential storage and session management
// ============================================================
#include <Arduino.h>

// ── Credential management ──────────────────────────────────

/**
 * On first boot: creates admin/admin123 in NVS if no
 * credentials exist yet.  Safe to call on every boot.
 */
void initDefaultCredentials();

/**
 * Validate a username + plaintext password against NVS.
 * Returns true if they match.
 */
bool validateCredentials(const String& username, const String& password);

/**
 * Overwrite stored credentials with new username + password.
 * Existing sessions are NOT invalidated here — call
 * destroyAllSessions() separately if needed.
 */
bool updateCredentials(const String& newUsername, const String& newPassword);

/** Return the currently stored username (never returns the password). */
String getStoredUsername();

// ── Session management ─────────────────────────────────────

/**
 * Generate a new random session token, store it in RAM,
 * and return the token string.
 */
String createSession();

/** Return true if the token exists in the active session map. */
bool validateSession(const String& token);

/** Remove a single session token. */
void destroySession(const String& token);

/** Remove all active sessions (e.g. after credential change). */
void destroyAllSessions();
