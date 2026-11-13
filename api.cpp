#include "api.h"
#include "auth.h"
#include "products.h"
#include "storage.h"
#include "vending_controller.h"
#include "display_manager.h"
#include "wifi_manager.h"
#include "config.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <esp_wifi.h>
#include <time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

static const char* JOURNAL_FILE = "/pending_redemption.json";
static SemaphoreHandle_t apiMutex;
struct ApiLock {
    ApiLock() { xSemaphoreTake(apiMutex, portMAX_DELAY); }
    ~ApiLock() { xSemaphoreGive(apiMutex); }
};
struct ActiveSession { String userId; bool locked = false, finishing = false; uint32_t expiresAt = 0; };
struct PendingDispense {
    String userId, productId;
    int targetCredits = 0, targetStock = 0;
    String targetDispenseDate;
    int targetDailyDispenses = 0;
    String targetCreditActivity;
    uint32_t targetCreditActivityAt = 0;
    uint32_t motorStartAt = 0, sensorDetectedAt = 0;
    uint8_t slot = 0;
    bool motorStarted = false, delivered = false, journalWritten = false;
};
static ActiveSession active;
static PendingDispense pending;
static String lastResultUser, lastResult;
static String bottleCreditUser;
static int bottleCreditsWaiting = 0;
static uint32_t storedBottleCount = 0;
static bool bottleCountSaveFailed = false;
static uint32_t lastCommitRetry = 0;
struct SeenClient { String mac; uint32_t lastSeen = 0; };
static SeenClient seenClients[AP_MAX_CONN];

static bool saveBottleCount(uint32_t count) {
    Preferences prefs;
    if (!prefs.begin("bottle-store", false)) return false;
    bool saved = prefs.putUInt("count", count) == sizeof(count);
    prefs.end();
    return saved;
}

static void sendJson(AsyncWebServerRequest* request, int status, JsonDocument& doc) {
    String out; serializeJson(doc, out);
    AsyncWebServerResponse* response = request->beginResponse(status, "application/json", out);
    response->addHeader("Cache-Control", "no-store");
    request->send(response);
}
static void sendError(AsyncWebServerRequest* request, int status, const String& error) {
    JsonDocument doc; doc["error"] = error; sendJson(request, status, doc);
}
static String tokenFor(AsyncWebServerRequest* request) {
    return request->hasHeader("X-Session-Token") ? request->getHeader("X-Session-Token")->value() : "";
}
static bool requireUser(AsyncWebServerRequest* request, User*& user) {
    user = sessionUser(tokenFor(request));
    if (!user) { sendError(request, 401, "unauthorized"); return false; }
    if (!expireInactiveCredits(*user)) { sendError(request, 500, "credit_expiry_save_failed"); return false; }
    return true;
}
static bool requireAdmin(AsyncWebServerRequest* request, User*& user) {
    if (!requireUser(request, user)) return false;
    if (user->role != "admin") { sendError(request, 403, "admin_required"); return false; }
    return true;
}
static bool readBody(AsyncWebServerRequest* request, uint8_t* data, size_t len,
                     size_t index, size_t total, JsonDocument& doc) {
    if (total > 4096) {
        if (index + len == total) sendError(request, 413, "body_too_large");
        return false;
    }
    if (index == 0) request->_tempObject = new String();
    String* body = static_cast<String*>(request->_tempObject);
    if (!body) return false;
    body->concat(reinterpret_cast<const char*>(data), len);
    if (index + len != total) return false;
    bool valid = deserializeJson(doc, *body) == DeserializationError::Ok;
    delete body; request->_tempObject = nullptr;
    if (!valid) sendError(request, 400, "invalid_json");
    return valid;
}
static void emptyBody(AsyncWebServerRequest* request) {
    if (request->contentLength() == 0) sendError(request, 400, "invalid_json");
}
static void userJson(JsonObject result, const User& user) {
    result["id"] = user.id; result["firstname"] = user.firstname;
    result["middlename"] = user.middlename; result["lastname"] = user.lastname;
    result["username"] = user.username; result["role"] = user.role;
    result["credits"] = user.credits; result["last_credit_activity"] = user.lastCreditActivity;
    String date; int dailyDispenses;
    if (getDailyDispenseCount(user, date, dailyDispenses)) result["daily_dispenses"] = dailyDispenses;
    result["created_at"] = user.createdAt;
}
static void productsJson(JsonArray output) {
    for (const Product& product : getProducts()) {
        JsonObject item = output.add<JsonObject>();
        item["id"] = product.id; item["name"] = product.name;
        item["credit_cost"] = product.creditCost; item["stock"] = product.stock;
        item["slot"] = product.slot;
    }
}
static bool writeJournal(const PendingDispense& transaction) {
    JsonDocument doc;
    doc["user_id"] = transaction.userId; doc["product_id"] = transaction.productId;
    doc["credits"] = transaction.targetCredits; doc["stock"] = transaction.targetStock;
    doc["daily_dispense_date"] = transaction.targetDispenseDate; doc["daily_dispense_count"] = transaction.targetDailyDispenses;
    doc["last_credit_activity"] = transaction.targetCreditActivity; doc["last_credit_activity_at"] = transaction.targetCreditActivityAt;
    String out; serializeJson(doc, out);
    return writeFile(JOURNAL_FILE, out);
}
bool recoverRedemption() {
    if (!fileExists(JOURNAL_FILE)) return true;
    JsonDocument doc;
    if (deserializeJson(doc, readFile(JOURNAL_FILE)) != DeserializationError::Ok) return false;
    User* user = findUserById(doc["user_id"] | "");
    Product* product = getProductById(doc["product_id"] | "");
    int credits = doc["credits"] | -1, stock = doc["stock"] | -1;
    if (!user || !product || credits < 0 || stock < 0) return false;
    user->credits = credits; product->stock = stock;
    user->dailyDispenseDate = doc["daily_dispense_date"] | user->dailyDispenseDate;
    user->dailyDispenses = doc["daily_dispense_count"] | user->dailyDispenses;
    user->lastCreditActivity = doc["last_credit_activity"] | user->lastCreditActivity;
    user->lastCreditActivityAt = doc["last_credit_activity_at"] | user->lastCreditActivityAt;
    return saveUsers() && saveProducts() && deleteFile(JOURNAL_FILE);
}
static bool commitDelivered() {
    if (!pending.journalWritten) {
        if (!writeJournal(pending)) return false;
        pending.journalWritten = true;
    }
    User* user = findUserById(pending.userId);
    Product* product = getProductById(pending.productId);
    if (!user || !product) return false;
    user->credits = pending.targetCredits;
    user->dailyDispenseDate = pending.targetDispenseDate;
    user->dailyDispenses = pending.targetDailyDispenses;
    user->lastCreditActivity = pending.targetCreditActivity;
    user->lastCreditActivityAt = pending.targetCreditActivityAt;
    product->stock = pending.targetStock;
    if (!saveUsers() || !saveProducts() || !deleteFile(JOURNAL_FILE)) return false;
    if (!appendDispenseLog(pending.userId, pending.productId, 1))
        Serial.println("[Vend] Delivery saved, but dispense log write failed.");
    displayDone(user->credits);
    lastResultUser = pending.userId; lastResult = "delivered";
    pending = {};
    return true;
}
static bool beginRedemption(User& user, Product& product, bool physicalButton = false) {
    if (pending.slot || bottleCreditsWaiting || fileExists(JOURNAL_FILE) || product.stock < 1 ||
        product.creditCost > user.credits) return false;
    String date; int dailyDispenses;
    if (!getDailyDispenseCount(user, date, dailyDispenses) || dailyDispenses >= DAILY_DISPENSE_LIMIT) return false;
    pending.userId = user.id; pending.productId = product.id;
    pending.targetCredits = user.credits - product.creditCost;
    pending.targetStock = product.stock - 1;
    pending.targetDispenseDate = date;
    pending.targetDailyDispenses = dailyDispenses + 1;
    if (!currentCreditActivity(pending.targetCreditActivity, pending.targetCreditActivityAt)) { pending = {}; return false; }
    pending.slot = product.slot;
    pending.motorStartAt = millis() + 700;
    lastResultUser = user.id; lastResult = "button_pressed";
    if (physicalButton) displayButtonPressed(product.slot, product.name);
    else displaySelectionReceived(product.slot, product.name);
    return true;
}

static void handleRegister(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    ApiLock lock; JsonDocument doc; if (!readBody(request, data, len, index, total, doc)) return;
    String error, firstname = doc["firstname"] | "", middlename = doc["middlename"] | "";
    String lastname = doc["lastname"] | "", username = doc["username"] | "", password = doc["password"] | "";
    firstname.trim(); middlename.trim(); lastname.trim(); username.trim();
    if (!registerUser(firstname, middlename, lastname, username, password, error)) { sendError(request, 400, error); return; }
    JsonDocument result; result["success"] = true; sendJson(request, 201, result);
}
static void handleLogin(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    ApiLock lock; JsonDocument doc; if (!readBody(request, data, len, index, total, doc)) return;
    String username = doc["username"] | "", password = doc["password"] | ""; username.trim();
    User* user = findUserByUsername(username);
    if (!user || !verifyUserPassword(*user, password)) { sendError(request, 401, "invalid_credentials"); return; }
    JsonDocument result; result["token"] = createSession(user->id);
    userJson(result["user"].to<JsonObject>(), *user); sendJson(request, 200, result);
}
static void handleLogout(AsyncWebServerRequest* request) {
    ApiLock lock; destroySession(tokenFor(request));
    JsonDocument result; result["success"] = true; sendJson(request, 200, result);
}
static void handleMe(AsyncWebServerRequest* request) {
    ApiLock lock; User* user; if (!requireUser(request, user)) return;
    JsonDocument result; userJson(result["user"].to<JsonObject>(), *user); sendJson(request, 200, result);
}
static void handlePassword(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    ApiLock lock; User* user; if (!requireUser(request, user)) return;
    JsonDocument doc; if (!readBody(request, data, len, index, total, doc)) return;
    if (!changeUserPassword(*user, doc["current_password"] | "", doc["new_password"] | "")) {
        sendError(request, 400, "password_change_failed"); return;
    }
    JsonDocument result; result["success"] = true; sendJson(request, 200, result);
}
static void handleProducts(AsyncWebServerRequest* request) {
    ApiLock lock; User* user; if (!requireUser(request, user)) return;
    JsonDocument result; productsJson(result["products"].to<JsonArray>()); sendJson(request, 200, result);
}
static void handleAddProduct(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    ApiLock lock; User* user; if (!requireAdmin(request, user)) return;
    JsonDocument doc; if (!readBody(request, data, len, index, total, doc)) return;
    if (pending.slot) { sendError(request, 409, "machine_busy"); return; }
    String id, name = doc["name"] | ""; name.trim();
    if (!addProduct(name, doc["credit_cost"] | -1, doc["stock"] | -1, id)) {
        sendError(request, 400, "invalid_product_or_storage_error"); return;
    }
    JsonDocument result; result["id"] = id; sendJson(request, 201, result);
}
static void handleUpdateProduct(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    ApiLock lock; User* user; if (!requireAdmin(request, user)) return;
    JsonDocument doc; if (!readBody(request, data, len, index, total, doc)) return;
    if (pending.slot) { sendError(request, 409, "machine_busy"); return; }
    String id = doc["id"] | "", name = doc["name"] | ""; name.trim();
    if (!updateProduct(id, name, doc["credit_cost"] | -1, doc["stock"] | -1)) {
        sendError(request, 400, "invalid_product_or_storage_error"); return;
    }
    JsonDocument result; result["success"] = true; sendJson(request, 200, result);
}
static void handleDeleteProduct(AsyncWebServerRequest* request) {
    ApiLock lock; User* user; if (!requireAdmin(request, user)) return;
    if (pending.slot) { sendError(request, 409, "machine_busy"); return; }
    if (!request->hasParam("id") || !deleteProduct(request->getParam("id")->value())) {
        sendError(request, 404, "product_not_found_or_storage_error"); return;
    }
    JsonDocument result; result["success"] = true; sendJson(request, 200, result);
}
static void handleStart(AsyncWebServerRequest* request) {
    ApiLock lock; User* user; if (!requireUser(request, user)) return;
    if (storedBottleCount >= BOTTLE_STORAGE_CAPACITY || bottleCountSaveFailed) { sendError(request, 409, "machine_full"); return; }
    uint32_t now = millis();
    if ((active.locked && (int32_t)(active.expiresAt - now) > 0 && active.userId != user->id) ||
        (bottleCreditsWaiting && bottleCreditUser != user->id) || pending.slot) {
        sendError(request, 409, "machine_busy"); return;
    }
    active.locked = true; active.userId = user->id; active.expiresAt = now + 60000;
    setBottleCollectionEnabled(true);
    displayCredits(user->credits);
    JsonDocument result; result["success"] = true; sendJson(request, 200, result);
}
static void handleEnd(AsyncWebServerRequest* request) {
    ApiLock lock; User* user; if (!requireUser(request, user)) return;
    if (active.locked && active.userId != user->id) { sendError(request, 403, "not_session_owner"); return; }
    if (!active.locked) { JsonDocument result; result["success"] = true; sendJson(request, 200, result); return; }
    if (active.finishing || bottleCreditsWaiting) { sendError(request, 409, "bottle_processing_pending"); return; }
    if (storedBottleCount >= BOTTLE_STORAGE_CAPACITY || bottleCountSaveFailed) { sendError(request, 409, "machine_full"); return; }
    int credits = finishBottleCollection();
    if (credits < 0) { sendError(request, 409, "invalid_bottle_pattern"); return; }
    if (credits == 0) {
        active = {}; setBottleCollectionEnabled(false); displayMessage("Closed machine is", "Ready");
        JsonDocument result; result["success"] = true; result["credits"] = 0; sendJson(request, 200, result); return;
    }
    active.finishing = true; active.expiresAt = millis() + 1000;
    JsonDocument result; result["processing"] = true; result["credits"] = credits; result["door_seconds"] = 10;
    sendJson(request, 202, result);
}
static void handleStatus(AsyncWebServerRequest* request) {
    ApiLock lock; User* user; if (!requireUser(request, user)) return;
    int32_t remaining = active.locked ? (int32_t)(active.expiresAt - millis()) : 0;
    JsonDocument result; result["locked"] = active.locked && remaining > 0;
    result["user_id"] = active.userId;
    result["seconds_remaining"] = remaining > 0 ? (remaining + 999) / 1000 : 0;
    result["finishing"] = active.finishing;
    result["door_seconds_remaining"] = bottleDoorSecondsRemaining();
    result["machine_full"] = storedBottleCount >= BOTTLE_STORAGE_CAPACITY || bottleCountSaveFailed;
    result["bottle_detected"] = active.locked && active.userId == user->id && remaining > 0 &&
                                !active.finishing && bottleCreditPatternDetected();
    result["credits"] = user->credits;
    uint8_t blockedSensors = blockedDropSensorMask();
    JsonArray blocked = result["blocked_drop_sensors"].to<JsonArray>();
    for (int i = 0; i < 4; ++i) if (blockedSensors & (1u << i)) blocked.add(i + 1);
    result["dispense"] = lastResultUser == user->id ? lastResult : "";
    sendJson(request, 200, result);
}
static void handleRedeem(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    ApiLock lock; User* user; if (!requireUser(request, user)) return;
    JsonDocument doc; if (!readBody(request, data, len, index, total, doc)) return;
    if ((int)(doc["qty"] | 0) != 1) { sendError(request, 400, "one_item_per_request"); return; }
    Product* product = getProductById(doc["product_id"] | "");
    if (!product || product->stock < 1 || product->creditCost > user->credits) {
        sendError(request, 400, "insufficient_credits_or_stock"); return;
    }
    String date; int dailyDispenses;
    if (!getDailyDispenseCount(*user, date, dailyDispenses)) { sendError(request, 503, "clock_not_synced"); return; }
    if (dailyDispenses >= DAILY_DISPENSE_LIMIT) { sendError(request, 429, "daily_dispense_limit_reached"); return; }
    if (pending.slot || bottleCreditsWaiting || fileExists(JOURNAL_FILE)) {
        sendError(request, 409, "machine_busy"); return;
    }
    uint8_t blockedSensors = blockedDropSensorMask();
    if (blockedSensors) {
        JsonDocument result;
        result["error"] = "drop_sensor_blocked";
        JsonArray sensors = result["sensors"].to<JsonArray>();
        for (int i = 0; i < 4; ++i) if (blockedSensors & (1u << i)) sensors.add(i + 1);
        sendJson(request, 409, result); return;
    }
    if (!beginRedemption(*user, *product)) { sendError(request, 409, "machine_unavailable"); return; }
    JsonDocument result; result["status"] = "dispensing"; sendJson(request, 202, result);
}
static void handleAdminUsers(AsyncWebServerRequest* request) {
    ApiLock lock; User* user; if (!requireAdmin(request, user)) return;
    JsonDocument result; JsonArray list = result["users"].to<JsonArray>();
    for (const User& item : getUsers()) userJson(list.add<JsonObject>(), item);
    sendJson(request, 200, result);
}
static void handleAdminLog(AsyncWebServerRequest* request) {
    ApiLock lock; User* user; if (!requireAdmin(request, user)) return;
    JsonDocument doc; if (deserializeJson(doc, dispenseLogJson()) != DeserializationError::Ok)
        doc["log"].to<JsonArray>();
    sendJson(request, 200, doc);
}
static void handleAdminCredits(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    ApiLock lock; User* user; if (!requireAdmin(request, user)) return;
    if (pending.slot) { sendError(request, 409, "machine_busy"); return; }
    JsonDocument doc; if (!readBody(request, data, len, index, total, doc)) return;
    User* target = findUserById(doc["user_id"] | "");
    int credits = doc["credits"] | -1;
    if (!target || credits < 0 || !setUserCredits(*target, credits)) {
        sendError(request, 400, "invalid_credits_or_storage_error"); return;
    }
    JsonDocument result; result["success"] = true; sendJson(request, 200, result);
}
static void handleAdminStorage(AsyncWebServerRequest* request) {
    ApiLock lock; User* user; if (!requireAdmin(request, user)) return;
    JsonDocument result; result["count"] = storedBottleCount; result["capacity"] = BOTTLE_STORAGE_CAPACITY;
    sendJson(request, 200, result);
}
static void handleAdminStorageReset(AsyncWebServerRequest* request) {
    ApiLock lock; User* user; if (!requireAdmin(request, user)) return;
    if (active.locked || bottleCreditsWaiting || pending.slot || !saveBottleCount(0)) {
        sendError(request, 409, "storage_reset_unavailable"); return;
    }
    storedBottleCount = 0; bottleCountSaveFailed = false; setBottleStorageFull(false);
    JsonDocument result; result["count"] = storedBottleCount; sendJson(request, 200, result);
}
static void handleAdminWiFi(AsyncWebServerRequest* request) {
    ApiLock lock; User* user; if (!requireAdmin(request, user)) return;
    String ssid, password; loadStationCredentials(ssid, password);
    JsonDocument result; result["ssid"] = ssid; result["connected"] = stationConnected(); result["connection_status"] = stationStatus(); result["time_synced"] = timeSynchronized();
    result["current_epoch"] = timeSynchronized() ? static_cast<uint32_t>(time(nullptr)) : 0;
    sendJson(request, 200, result);
}
static void handleAdminClients(AsyncWebServerRequest* request) {
    ApiLock lock; User* user; if (!requireAdmin(request, user)) return;
    wifi_sta_list_t stations{};
    if (esp_wifi_ap_get_sta_list(&stations) != ESP_OK) { sendError(request, 500, "client_list_unavailable"); return; }
    JsonDocument result;
    JsonArray clients = result["clients"].to<JsonArray>();
    for (int i = 0; i < stations.num; ++i) {
        char mac[18];
        snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
            stations.sta[i].mac[0], stations.sta[i].mac[1], stations.sta[i].mac[2],
            stations.sta[i].mac[3], stations.sta[i].mac[4], stations.sta[i].mac[5]);
        int slot = -1;
        for (int j = 0; j < AP_MAX_CONN; ++j) if (seenClients[j].mac == mac) { slot = j; break; }
        if (slot < 0) for (int j = 0; j < AP_MAX_CONN; ++j) if (seenClients[j].mac.isEmpty()) { slot = j; break; }
        if (slot < 0) {
            slot = 0;
            for (int j = 1; j < AP_MAX_CONN; ++j) if ((int32_t)(seenClients[j].lastSeen - seenClients[slot].lastSeen) < 0) slot = j;
        }
        seenClients[slot].mac = mac; seenClients[slot].lastSeen = millis();
    }
    for (const SeenClient& client : seenClients) if (!client.mac.isEmpty()) {
        JsonObject item = clients.add<JsonObject>();
        item["mac"] = client.mac;
        item["connected"] = millis() - client.lastSeen < 5000;
    }
    int32_t remaining = active.locked ? (int32_t)(active.expiresAt - millis()) : 0;
    User* activeUser = active.locked && remaining > 0 ? findUserById(active.userId) : nullptr;
    result["insert_available"] = !activeUser;
    result["active_username"] = activeUser ? activeUser->firstname + " " + activeUser->lastname : "";
    result["remaining_seconds"] = remaining > 0 ? (remaining + 999) / 1000 : 0;
    sendJson(request, 200, result);
}
static void handleAdminWiFiSave(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    ApiLock lock; User* user; if (!requireAdmin(request, user)) return;
    JsonDocument doc; if (!readBody(request, data, len, index, total, doc)) return;
    String ssid = doc["ssid"] | "", password;
    ssid.trim();
    if (!doc["password"].isNull()) password = doc["password"].as<String>();
    else {
        String savedSsid; loadStationCredentials(savedSsid, password);
        if (ssid != savedSsid) { sendError(request, 400, "wifi_password_required"); return; }
    }
    if (!saveStationCredentials(ssid, password)) { sendError(request, 400, "invalid_wifi_settings"); return; }
    connectToStation(ssid, password);
    JsonDocument result; result["status"] = "connecting"; sendJson(request, 200, result);
}
static void handleAdminTimeSync(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    ApiLock lock; User* user; if (!requireAdmin(request, user)) return;
    JsonDocument doc; if (!readBody(request, data, len, index, total, doc)) return;
    uint32_t epoch = doc["epoch"] | 0UL;
    if (!setSystemTime(epoch)) { sendError(request, 400, "invalid_time"); return; }
    JsonDocument result; result["time_synced"] = true; sendJson(request, 200, result);
}

void updateReverseVending() {
    ApiLock lock;
    uint32_t now = millis();
    if (active.locked && active.finishing) {
        if (bottleDetectionPending() || bottleCreditsWaiting) active.expiresAt = now + 1000;
        else { active = {}; setBottleCollectionEnabled(false); }
    } else if (active.locked && (int32_t)(active.expiresAt - now) <= 0) {
        if (bottleDetectionPending() || bottleCreditsWaiting) active.expiresAt = now + 1000;
        else { active = {}; setBottleCollectionEnabled(false); }
    }
    int creditsEarned;
    if (active.locked && consumeValidatedBottle(creditsEarned)) {
        if (storedBottleCount < BOTTLE_STORAGE_CAPACITY) {
            ++storedBottleCount;
            bottleCountSaveFailed = !saveBottleCount(storedBottleCount);
            setBottleStorageFull(storedBottleCount >= BOTTLE_STORAGE_CAPACITY || bottleCountSaveFailed);
            if (bottleCountSaveFailed) Serial.println("[Storage] Bottle count save failed; collection stopped.");
            bottleCreditUser = active.userId;
            bottleCreditsWaiting += creditsEarned;
            Serial.printf("[Bottle] Validated; awarding %d credits to %s.\n", creditsEarned, bottleCreditUser.c_str());
        }
        if (!active.finishing) active.expiresAt = now + 60000;
    }
    if (bottleCreditsWaiting) {
        User* user = findUserById(bottleCreditUser);
        int creditsToApply = bottleCreditsWaiting;
        if (user && addUserCredits(*user, creditsToApply)) {
            bottleCreditsWaiting = 0;
            if (active.finishing) startBottleDoorCycle();
            else displayCredits(user->credits);
            Serial.printf("[Bottle] Added %d credits; total is %d.\n", creditsToApply, user->credits);
        }
    }
    DispenseResult result = takeDispenseResult();
    if (pending.slot && result == DispenseResult::Delivered) {
        pending.delivered = true;
        pending.sensorDetectedAt = now;
        lastResultUser = pending.userId; lastResult = "saving";
        displaySensorDetected(pending.slot);
    }
    if (pending.slot && result == DispenseResult::TimedOut) {
        displayError(pending.slot);
        lastResultUser = pending.userId; lastResult = "timeout";
        pending = {};
    }
    if (pending.slot && !pending.motorStarted && !pending.delivered &&
        (int32_t)(now - pending.motorStartAt) >= 0) {
        uint8_t blockedSensors = blockedDropSensorMask();
        if (blockedSensors) {
            String sensors = "IR blocked:";
            for (int i = 0; i < 4; ++i) if (blockedSensors & (1u << i)) sensors += " " + String(i + 1);
            displayMessage("SENSOR BLOCKED", sensors);
            lastResultUser = pending.userId; lastResult = "sensor_blocked";
            pending = {};
        } else {
            Product* product = getProductById(pending.productId);
            if (!product || !startDispense(pending.slot)) {
                displayMessage("MOTOR NOT STARTED", "PLEASE TRY AGAIN");
                lastResultUser = pending.userId; lastResult = "motor_unavailable";
                pending = {};
            } else {
                pending.motorStarted = true;
                lastResultUser = pending.userId; lastResult = "dispensing";
                displayMotorRunning(pending.slot, product->name);
            }
        }
    }
    if (pending.slot && pending.delivered && now - pending.sensorDetectedAt >= 1000 &&
        now - lastCommitRetry >= 1000) {
        lastCommitRetry = now;
        if (!commitDelivered()) Serial.println("[Vend] Retrying delivery record write.");
    }
    uint8_t pressed = takeButtonPress();
    if (pressed && !active.locked) {
        displayMessage("SIGN IN ON PORTAL", "THEN INSERT BOTTLE");
    } else if (pressed && pending.slot) {
        displayMessage("VENDING IN PROGRESS", "PLEASE WAIT");
    } else if (pressed && active.locked) {
        uint8_t blockedSensors = blockedDropSensorMask();
        if (blockedSensors) {
            String sensors = "IR blocked:";
            for (int i = 0; i < 4; ++i) if (blockedSensors & (1u << i)) sensors += " " + String(i + 1);
            displayMessage("CHECK DROP SENSOR", sensors);
        }
        User* user = findUserById(active.userId);
        Product* product = nullptr;
        for (const Product& item : getProducts()) if (item.slot == pressed) product = getProductById(item.id);
        if (!blockedSensors && user && product) {
            if (product->stock < 1) displayEmpty(pressed, product->name);
            else if (product->creditCost > user->credits) displayNoCredit(product->creditCost, user->credits);
            else {
                String date; int dailyDispenses;
                if (!getDailyDispenseCount(*user, date, dailyDispenses)) displayMessage("CLOCK NOT READY", "OPEN ADMIN DASHBOARD");
                else if (dailyDispenses >= DAILY_DISPENSE_LIMIT) displayMessage("DAILY LIMIT", "4 ITEMS TODAY");
                else if (!beginRedemption(*user, *product, true)) displayMessage("VENDING UNAVAILABLE", "PLEASE TRY AGAIN");
            }
        } else if (!blockedSensors && (!user || !product)) {
            displayMessage("PRODUCT UNAVAILABLE", "CHECK SLOT SETUP");
        }
    }
}

bool setupAPI(AsyncWebServer& server) {

    // Initialize API mutex
    if (apiMutex == nullptr) {
        apiMutex = xSemaphoreCreateMutex();
    }

    if (apiMutex == nullptr) {
        Serial.println("ERROR: Failed to create API mutex");
        return false;
    }

    Preferences prefs;
    if (!prefs.begin("bottle-store", false)) {
        Serial.println("[Storage] ERROR: Cannot open bottle-store namespace.");
        return false;
    }
    if (!prefs.isKey("count") && prefs.putUInt("count", 0) != sizeof(uint32_t)) {
        Serial.println("[Storage] ERROR: Cannot initialize bottle count.");
        prefs.end();
        return false;
    }
    storedBottleCount = prefs.getUInt("count", 0);
    prefs.end();

    bottleCountSaveFailed = storedBottleCount > BOTTLE_STORAGE_CAPACITY;
    setBottleStorageFull(storedBottleCount >= BOTTLE_STORAGE_CAPACITY || bottleCountSaveFailed);
    Serial.printf("[Storage] Stored bottle count: %u\n", storedBottleCount);
    // =========================
    // AUTHENTICATION API
    // =========================

    server.on("/api/register", HTTP_POST,
        emptyBody, nullptr, handleRegister);

    server.on("/api/login", HTTP_POST,
        emptyBody, nullptr, handleLogin);

    server.on("/api/logout", HTTP_POST,
        handleLogout);

    server.on("/api/me", HTTP_GET,
        handleMe);

    server.on("/api/user/password", HTTP_POST,
        emptyBody, nullptr, handlePassword);


    // =========================
    // PRODUCT API
    // =========================

    server.on("/api/products", HTTP_GET,
        handleProducts);

    server.on("/api/products", HTTP_POST,
        emptyBody, nullptr, handleAddProduct);

    server.on("/api/products", HTTP_PUT,
        emptyBody, nullptr, handleUpdateProduct);

    server.on("/api/products", HTTP_DELETE,
        handleDeleteProduct);


    // =========================
    // SESSION API
    // =========================

    server.on("/api/session/start", HTTP_POST,
        handleStart);

    server.on("/api/session/end", HTTP_POST,
        handleEnd);

    server.on("/api/session/status", HTTP_GET,
        handleStatus);


    // =========================
    // REDEEM API
    // =========================

    server.on("/api/redeem", HTTP_POST,
        emptyBody, nullptr, handleRedeem);


    // =========================
    // ADMIN API
    // =========================

    server.on("/api/admin/users", HTTP_GET,
        handleAdminUsers);
    server.on("/api/admin/clients", HTTP_GET,
        handleAdminClients);

    server.on("/api/admin/log", HTTP_GET,
        handleAdminLog);

    server.on("/api/admin/credits", HTTP_POST,
        emptyBody, nullptr, handleAdminCredits);

    server.on("/api/admin/storage", HTTP_GET,
        handleAdminStorage);

    server.on("/api/admin/storage/reset", HTTP_POST,
        handleAdminStorageReset);

    server.on("/api/admin/wifi", HTTP_GET,
        handleAdminWiFi);

    server.on("/api/admin/wifi", HTTP_POST,
        emptyBody, nullptr, handleAdminWiFiSave);

    server.on("/api/admin/time", HTTP_POST,
        emptyBody, nullptr, handleAdminTimeSync);


    // =========================
    // HTML PAGES
    // =========================

    server.on("/", HTTP_GET,
        [](AsyncWebServerRequest* request) {
            request->redirect("/login.html");
        });

    server.on("/login.html", HTTP_GET,
        [](AsyncWebServerRequest* request) {
            request->send(LittleFS, "/login.html", "text/html");
        });

    server.on("/test.html", HTTP_GET,
        [](AsyncWebServerRequest* request) {
            request->send(LittleFS, "/test.html", "text/html");
        });

    server.on("/admin-test.html", HTTP_GET,
        [](AsyncWebServerRequest* request) {
            request->send(LittleFS, "/admin-test.html", "text/html");
        });

    server.on("/login-test.html", HTTP_GET,
        [](AsyncWebServerRequest* request) {
            request->send(LittleFS, "/login-test.html", "text/html");
        });


    // =========================
    // IMAGE FILES
    // =========================

    server.on("/product-sprite.png", HTTP_GET,
        [](AsyncWebServerRequest* request) {
            request->send(LittleFS, "/product-sprite.png", "image/png");
        });

    server.on("/eco-background.jpg", HTTP_GET,
        [](AsyncWebServerRequest* request) {
            request->send(LittleFS, "/eco-background.jpg", "image/jpeg");
        });

    server.on("/login-background.jpg", HTTP_GET,
        [](AsyncWebServerRequest* request) {
            request->send(LittleFS, "/login-background.jpg", "image/jpeg");
        });


    // =========================
    // PORTAL AND ADMIN PAGES
    // =========================

    server.on("/portal", HTTP_GET,
        [](AsyncWebServerRequest* request) {
            request->send(LittleFS, "/portal.html", "text/html");
        });

    server.on("/admin", HTTP_GET,
        [](AsyncWebServerRequest* request) {
            request->send(LittleFS, "/admin.html", "text/html");
        });


    // =========================
    // JAVASCRIPT FILES
    // =========================

    server.on("/js/auth.js", HTTP_GET,
        [](AsyncWebServerRequest* request) {
            request->send(LittleFS, "/js/auth.js", "text/javascript");
        });


    // =========================
    // 404 HANDLER
    // =========================

    server.onNotFound([](AsyncWebServerRequest* request) {
        if (request->url().startsWith("/api/")) sendError(request, 404, "not_found");
        else request->redirect("/login.html");
    });

    Serial.println("API routes registered successfully.");

    return true;
}
