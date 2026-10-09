#include "auth.h"
// Guide: Namamahala ito sa user accounts, password verification,
// credits, dispense log at session tokens. Users at log ay naka-save sa LittleFS;
// ang active sessions ay nasa RAM kaya nawawala kapag nag-restart ang ESP32.
#include "storage.h"
#include <ArduinoJson.h>
#include <mbedtls/sha256.h>
#include <esp_random.h>
#include <map>
#include <time.h>
#include <limits.h>

static const char* USERS_FILE = "/data/users.json";
static const char* LOG_FILE = "/data/dispense_log.json";
static std::vector<User> users;
static std::map<String, String> sessions; // RAM only by design
static std::map<String, uint32_t> sessionTouched;
static const uint32_t SESSION_EXPIRY_MS = 8UL * 60UL * 60UL * 1000UL;

static String randomHex(size_t bytes = 16) {
    String out; out.reserve(bytes * 2);
    for (size_t i = 0; i < bytes; ++i) { char part[3]; snprintf(part, sizeof(part), "%02x", (unsigned)(esp_random() & 0xff)); out += part; }
    return out;
}
static String nowIso() {
    time_t now = time(nullptr); if (now < 1700000000) return "";
    struct tm value; gmtime_r(&now, &value); char out[25]; strftime(out, sizeof(out), "%Y-%m-%dT%H:%M:%SZ", &value); return out;
}
static String passwordHash(const String& password, const String& salt) {
    uint8_t digest[32]; String input = salt + ":" + password; mbedtls_sha256_context context; mbedtls_sha256_init(&context);
    mbedtls_sha256_starts(&context, 0); mbedtls_sha256_update(&context, reinterpret_cast<const unsigned char*>(input.c_str()), input.length()); mbedtls_sha256_finish(&context, digest); mbedtls_sha256_free(&context);
    String out; out.reserve(64); for (uint8_t byte : digest) { char hex[3]; snprintf(hex, sizeof(hex), "%02x", byte); out += hex; } return out;
}
static bool sameConstantTime(const String& a, const String& b) {
    if (a.length() != b.length()) return false; uint8_t difference = 0;
    for (size_t i = 0; i < a.length(); ++i) difference |= a[i] ^ b[i]; return difference == 0;
}
static void writeUser(JsonObject object, const User& user) {
    object["id"] = user.id; object["firstname"] = user.firstname; object["middlename"] = user.middlename; object["lastname"] = user.lastname;
    object["email"] = user.email; object["password_hash"] = user.passwordHash; object["salt"] = user.salt; object["role"] = user.role;
    object["credits"] = user.credits; object["last_credit_activity"] = user.lastCreditActivity; object["created_at"] = user.createdAt;
}
bool saveUsers() {
    JsonDocument doc; JsonArray list = doc["users"].to<JsonArray>(); for (const User& user : users) writeUser(list.add<JsonObject>(), user);
    String out; serializeJson(doc, out); return writeFile(USERS_FILE, out);
}
bool initUsers() {
    users.clear(); String json = readFile(USERS_FILE);
    if (fileExists(USERS_FILE) && json.isEmpty()) return false;
    if (!json.isEmpty()) { JsonDocument doc; if (deserializeJson(doc, json) != DeserializationError::Ok || !doc["users"].is<JsonArray>()) return false;
      for (JsonObject item : doc["users"].as<JsonArray>()) {
        User user; user.id = item["id"] | ""; user.firstname = item["firstname"] | ""; user.middlename = item["middlename"] | "";
        user.lastname = item["lastname"] | ""; user.email = item["email"] | ""; user.passwordHash = item["password_hash"] | ""; user.salt = item["salt"] | "";
        user.role = item["role"] | "user"; user.credits = item["credits"] | 0; user.lastCreditActivity = item["last_credit_activity"] | ""; user.createdAt = item["created_at"] | "";
        if (user.id.isEmpty() || user.email.isEmpty() || user.salt.isEmpty()) return false;
        users.push_back(user);
    }}
    if (users.empty()) {
        if (fileExists(USERS_FILE)) return false;
        User admin; admin.id = randomHex(); admin.firstname = "System"; admin.lastname = "Administrator"; admin.email = "admin@local";
        String initialPassword = randomHex(12);
        admin.salt = randomHex(); admin.passwordHash = passwordHash(initialPassword, admin.salt); admin.role = "admin"; admin.createdAt = nowIso(); users.push_back(admin);
        if (!saveUsers()) return false;
        Serial.printf("[Auth] Initial admin login: admin@local / %s\n", initialPassword.c_str());
    }
    for (User& user : users) if (user.role == "admin" && verifyUserPassword(user, "admin123")) {
        String replacement = randomHex(12);
        String oldSalt = user.salt, oldHash = user.passwordHash;
        user.salt = randomHex(); user.passwordHash = passwordHash(replacement, user.salt);
        if (!saveUsers()) { user.salt = oldSalt; user.passwordHash = oldHash; return false; }
        Serial.printf("[Auth] Replaced known admin password for %s: %s\n", user.email.c_str(), replacement.c_str());
    }
    return true;
}
User* findUserByEmail(const String& email) { for (User& user : users) if (user.email.equalsIgnoreCase(email)) return &user; return nullptr; }
User* findUserById(const String& id) { for (User& user : users) if (user.id == id) return &user; return nullptr; }
const std::vector<User>& getUsers() { return users; }
bool registerUser(const String& firstname, const String& middlename, const String& lastname, const String& email, const String& password, String& error) {
    if (firstname.isEmpty() || lastname.isEmpty() || email.indexOf('@') < 1 || password.length() < 8) { error = "First name, last name, valid email, and an 8-character password are required"; return false; }
    if (findUserByEmail(email)) { error = "Email is already registered"; return false; }
    User user; user.id = randomHex(); user.firstname = firstname; user.middlename = middlename; user.lastname = lastname; user.email = email; user.salt = randomHex();
    user.passwordHash = passwordHash(password, user.salt); user.role = "user"; user.createdAt = nowIso(); users.push_back(user);
    if (!saveUsers()) { users.pop_back(); error = "Could not save account"; return false; } return true;
}
bool changeUserPassword(User& user, const String& currentPassword, const String& newPassword) {
    if (newPassword.length() < 8 || !sameConstantTime(user.passwordHash, passwordHash(currentPassword, user.salt))) return false;
    String oldSalt = user.salt, oldHash = user.passwordHash;
    user.salt = randomHex(); user.passwordHash = passwordHash(newPassword, user.salt);
    if (saveUsers()) return true;
    user.salt = oldSalt; user.passwordHash = oldHash;
    return false;
}
bool verifyUserPassword(const User& user, const String& password) { return sameConstantTime(user.passwordHash, passwordHash(password, user.salt)); }
bool setUserCredits(User& user, int amount) {
    if (amount < 0) return false;
    int previous = user.credits; String previousActivity = user.lastCreditActivity;
    user.credits = amount; user.lastCreditActivity = nowIso();
    if (saveUsers()) return true;
    user.credits = previous; user.lastCreditActivity = previousActivity;
    return false;
}
bool addUserCredits(User& user, int amount) {
    if (amount < 0 || user.credits > INT_MAX - amount) return false;
    return setUserCredits(user, user.credits + amount);
}
static void purgeSessions() { uint32_t now = millis(); for (auto it = sessionTouched.begin(); it != sessionTouched.end();) { if (now - it->second > SESSION_EXPIRY_MS) { sessions.erase(it->first); it = sessionTouched.erase(it); } else ++it; } }
String createSession(const String& userId) { purgeSessions(); String token = randomHex(32); sessions[token] = userId; sessionTouched[token] = millis(); return token; }
User* sessionUser(const String& token) { purgeSessions(); auto it = sessions.find(token); if (it == sessions.end()) return nullptr; sessionTouched[token] = millis(); return findUserById(it->second); }
void destroySession(const String& token) { sessions.erase(token); sessionTouched.erase(token); }
bool appendDispenseLog(const String& userId, const String& productId, int qty) {
    JsonDocument doc; String existing = readFile(LOG_FILE); if (!existing.isEmpty() && deserializeJson(doc, existing) != DeserializationError::Ok) return false;
    JsonArray log = doc["log"].is<JsonArray>() ? doc["log"].as<JsonArray>() : doc["log"].to<JsonArray>();
    // ponytail: keep the latest 100 entries; move to a separate log store if longer history is required.
    if (log.size() >= 100) log.remove(0);
    JsonObject item = log.add<JsonObject>(); item["user_id"] = userId; item["product_id"] = productId; item["qty"] = qty; item["timestamp"] = nowIso(); item["uptime_ms"] = millis();
    String out; serializeJson(doc, out); return writeFile(LOG_FILE, out);
}
String dispenseLogJson() { String log = readFile(LOG_FILE); return log.isEmpty() ? "{\"log\":[]}" : log; }
