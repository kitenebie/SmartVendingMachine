// ============================================================
//  auth.cpp — Credential storage (NVS/Preferences) + sessions
// ============================================================
#include "auth.h"
#include <Preferences.h>
#include <map>
#include <esp_random.h>

// NVS namespace and keys
static const char* NVS_NS   = "inv_auth";
static const char* KEY_USER = "username";
static const char* KEY_PASS = "password";

// In-RAM session store: token -> creation-time (ms)
static std::map<String, uint32_t> s_sessions;

// Session expiry: 8 hours
static const uint32_t SESSION_EXPIRY_MS = 8UL * 60UL * 60UL * 1000UL;

// ── Internal helpers ───────────────────────────────────────

static String generateToken() {
    String token = "";
    token.reserve(64);
    for (int i = 0; i < 32; i++) {
        uint32_t r = esp_random();
        char buf[3];
        snprintf(buf, sizeof(buf), "%02x", (uint8_t)(r & 0xFF));
        token += buf;
    }
    return token;
}

// Remove sessions older than SESSION_EXPIRY_MS
static void purgeExpiredSessions() {
    uint32_t now = millis();
    for (auto it = s_sessions.begin(); it != s_sessions.end(); ) {
        if (now - it->second > SESSION_EXPIRY_MS) {
            it = s_sessions.erase(it);
        } else {
            ++it;
        }
    }
}

// ── Public API ─────────────────────────────────────────────

void initDefaultCredentials() {
    Preferences prefs;
    prefs.begin(NVS_NS, false);     // read-write

    bool hasUser = prefs.isKey(KEY_USER);
    bool hasPass = prefs.isKey(KEY_PASS);

    if (!hasUser || !hasPass) {
        Serial.println("[Auth] No credentials found — creating default account");
        prefs.putString(KEY_USER, "admin");
        prefs.putString(KEY_PASS, "admin123");
        Serial.println("[Auth] Default account created: admin / admin123");
    } else {
        Serial.printf("[Auth] Loaded credentials for user: %s\n",
                      prefs.getString(KEY_USER, "").c_str());
    }

    prefs.end();
}

bool validateCredentials(const String& username, const String& password) {
    Preferences prefs;
    prefs.begin(NVS_NS, true);      // read-only
    String storedUser = prefs.getString(KEY_USER, "");
    String storedPass = prefs.getString(KEY_PASS, "");
    prefs.end();

    // Constant-time comparison is ideal; for an embedded single-user
    // system this equality check is sufficient.
    return (username == storedUser && password == storedPass);
}

bool updateCredentials(const String& newUsername, const String& newPassword) {
    if (newUsername.isEmpty() || newPassword.isEmpty()) return false;

    Preferences prefs;
    prefs.begin(NVS_NS, false);
    prefs.putString(KEY_USER, newUsername);
    prefs.putString(KEY_PASS, newPassword);
    prefs.end();

    Serial.printf("[Auth] Credentials updated for user: %s\n", newUsername.c_str());
    return true;
}

String getStoredUsername() {
    Preferences prefs;
    prefs.begin(NVS_NS, true);
    String u = prefs.getString(KEY_USER, "");
    prefs.end();
    return u;
}

// ── Sessions ───────────────────────────────────────────────

String createSession() {
    purgeExpiredSessions();
    String token = generateToken();
    s_sessions[token] = millis();
    Serial.printf("[Auth] Session created (total: %d)\n", (int)s_sessions.size());
    return token;
}

bool validateSession(const String& token) {
    if (token.isEmpty()) return false;
    purgeExpiredSessions();

    auto it = s_sessions.find(token);
    if (it == s_sessions.end()) return false;

    // Refresh session timestamp on each valid access
    it->second = millis();
    return true;
}

void destroySession(const String& token) {
    s_sessions.erase(token);
}

void destroyAllSessions() {
    s_sessions.clear();
    Serial.println("[Auth] All sessions destroyed");
}
