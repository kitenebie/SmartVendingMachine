#include "api.h"
// Guide: Dito nakakabit ang web API routes para sa login, users,
// products, credits, bottle collection at redemption. I-check muna ang session
// o admin role bago magbasa o magbago ng data; setupAPI() ang nagrerehistro nito.
#include "auth.h"
#include "products.h"
#include "vending_controller.h"
#include <ArduinoJson.h>
#include <LittleFS.h>

static const int CREDIT_PER_BOTTLE = 1; // Tune this to match the program's bottle value.
struct ActiveSession { String userId; bool locked = false; uint32_t expiresAt = 0; } active;
static uint32_t lastExpirySweep = 0;

static void sendJson(AsyncWebServerRequest* request, int status, JsonDocument& doc) { String out; serializeJson(doc, out); AsyncWebServerResponse* response = request->beginResponse(status, "application/json", out); response->addHeader("Cache-Control", "no-store"); request->send(response); }
static void sendError(AsyncWebServerRequest* request, int status, const String& error) { JsonDocument doc; doc["error"] = error; sendJson(request, status, doc); }
static String tokenFor(AsyncWebServerRequest* request) { return request->hasHeader("X-Session-Token") ? request->getHeader("X-Session-Token")->value() : ""; }
static User* currentUser(AsyncWebServerRequest* request) { return sessionUser(tokenFor(request)); }
static bool requireUser(AsyncWebServerRequest* request, User*& user) { user = currentUser(request); if (!user) { sendError(request, 401, "unauthorized"); return false; } return true; }
static bool requireAdmin(AsyncWebServerRequest* request, User*& user) { if (!requireUser(request, user)) return false; if (user->role != "admin") { sendError(request, 403, "admin_required"); return false; } return true; }
static bool readBody(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total, JsonDocument& doc) {
    if (index == 0) request->_tempObject = new String();
    String* body = static_cast<String*>(request->_tempObject); if (!body) return false;
    body->concat(reinterpret_cast<char*>(data), len);
    if (index + len != total) return false;
    bool ok = deserializeJson(doc, *body) == DeserializationError::Ok; delete body; request->_tempObject = nullptr; return ok;
}
static void userJson(JsonObject result, const User& user) {
    result["id"] = user.id; result["firstname"] = user.firstname; result["middlename"] = user.middlename; result["lastname"] = user.lastname;
    result["email"] = user.email; result["role"] = user.role; result["credits"] = user.credits; result["last_credit_activity"] = user.lastCreditActivity; result["created_at"] = user.createdAt;
}
static void productsJson(JsonArray output) { for (const Product& product : getProducts()) { JsonObject item = output.add<JsonObject>(); item["id"] = product.id; item["name"] = product.name; item["credit_cost"] = product.creditCost; item["stock"] = product.stock; } }
static void requireValidBody(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total, JsonDocument& doc, bool& ready) { ready = readBody(request, data, len, index, total, doc); if (index + len == total && !ready) sendError(request, 400, "invalid_json"); }

static void handleRegister(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    JsonDocument doc; bool ready; requireValidBody(request, data, len, index, total, doc, ready); if (!ready) return;
    String error, firstname = doc["firstname"] | "", middlename = doc["middlename"] | "", lastname = doc["lastname"] | "", email = doc["email"] | "", password = doc["password"] | "";
    firstname.trim(); middlename.trim(); lastname.trim(); email.trim(); email.toLowerCase();
    if (!registerUser(firstname, middlename, lastname, email, password, error)) { sendError(request, 400, error); return; }
    JsonDocument result; result["success"] = true; sendJson(request, 201, result);
}
static void handleLogin(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    JsonDocument doc; bool ready; requireValidBody(request, data, len, index, total, doc, ready); if (!ready) return;
    String email = doc["email"] | "", password = doc["password"] | ""; email.trim(); email.toLowerCase(); User* user = findUserByEmail(email);
    // Reuse the password-change verifier only indirectly would mutate data; compare hashed value here through a zero-change check is not safe.
    if (!user || password.isEmpty()) { sendError(request, 401, "invalid_credentials"); return; }
    // The stored hash format is deliberately checked inside this small local verifier via a password-change dry run avoidance.
    if (!verifyUserPassword(*user, password)) { sendError(request, 401, "invalid_credentials"); return; }
    JsonDocument result; result["token"] = createSession(user->id); userJson(result["user"].to<JsonObject>(), *user); sendJson(request, 200, result);
}
static void handleLogout(AsyncWebServerRequest* request) { destroySession(tokenFor(request)); JsonDocument result; result["success"] = true; sendJson(request, 200, result); }
static void handleMe(AsyncWebServerRequest* request) { User* user; if (!requireUser(request, user)) return; JsonDocument result; userJson(result["user"].to<JsonObject>(), *user); sendJson(request, 200, result); }
static void handlePassword(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    User* user; if (!requireUser(request, user)) return; JsonDocument doc; bool ready; requireValidBody(request, data, len, index, total, doc, ready); if (!ready) return;
    if (!changeUserPassword(*user, doc["current_password"] | "", doc["new_password"] | "")) { sendError(request, 400, "current password is incorrect or new password is too short"); return; }
    JsonDocument result; result["success"] = true; sendJson(request, 200, result);
}
static void handleProducts(AsyncWebServerRequest* request) { User* user; if (!requireUser(request, user)) return; JsonDocument result; productsJson(result["products"].to<JsonArray>()); sendJson(request, 200, result); }
static void handleAddProduct(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    User* user; if (!requireAdmin(request, user)) return; JsonDocument doc; bool ready; requireValidBody(request, data, len, index, total, doc, ready); if (!ready) return;
    if (getProducts().size() >= 4) { sendError(request, 400, "machine_has_four_dispense_slots"); return; }
    String id, name = doc["name"] | ""; name.trim(); if (!addProduct(name, doc["credit_cost"] | -1, doc["stock"] | -1, id)) { sendError(request, 400, "invalid_product"); return; }
    JsonDocument result; result["id"] = id; sendJson(request, 201, result);
}
static void handleUpdateProduct(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    User* user; if (!requireAdmin(request, user)) return; JsonDocument doc; bool ready; requireValidBody(request, data, len, index, total, doc, ready); if (!ready) return;
    String id = doc["id"] | "", name = doc["name"] | ""; name.trim(); if (!updateProduct(id, name, doc["credit_cost"] | -1, doc["stock"] | -1)) { sendError(request, 400, "invalid_product"); return; }
    JsonDocument result; result["success"] = true; sendJson(request, 200, result);
}
static void handleDeleteProduct(AsyncWebServerRequest* request) { User* user; if (!requireAdmin(request, user)) return; if (!request->hasParam("id") || !deleteProduct(request->getParam("id")->value())) { sendError(request, 404, "product_not_found"); return; } JsonDocument result; result["success"] = true; sendJson(request, 200, result); }
static void handleStart(AsyncWebServerRequest* request) {
    User* user; if (!requireUser(request, user)) return; uint32_t now = millis();
    if (active.locked && (int32_t)(active.expiresAt - now) > 0 && active.userId != user->id) { sendError(request, 409, "machine_busy"); return; }
    active.locked = true; active.userId = user->id; active.expiresAt = now + 60000; setBottleCollectionEnabled(true); JsonDocument result; result["success"] = true; sendJson(request, 200, result);
}
static void handleEnd(AsyncWebServerRequest* request) { User* user; if (!requireUser(request, user)) return; if (active.locked && active.userId != user->id) { sendError(request, 403, "not_session_owner"); return; } active = {}; setBottleCollectionEnabled(false); JsonDocument result; result["success"] = true; sendJson(request, 200, result); }
static void handleStatus(AsyncWebServerRequest* request) {
    User* user; if (!requireUser(request, user)) return; uint32_t now = millis(); JsonDocument result; result["locked"] = active.locked; result["user_id"] = active.userId;
    result["seconds_remaining"] = active.locked ? (uint32_t)((active.expiresAt - now + 999) / 1000) : 0; result["credits"] = user->credits; sendJson(request, 200, result);
}
static void handleRedeem(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    User* user; if (!requireUser(request, user)) return; JsonDocument doc; bool ready; requireValidBody(request, data, len, index, total, doc, ready); if (!ready) return;
    String id = doc["product_id"] | ""; int qty = doc["qty"] | 0; Product* product = getProductById(id); if (!product || qty < 1 || qty > product->stock || qty * product->creditCost > user->credits) { sendError(request, 400, "insufficient_credits_or_stock"); return; }
    uint8_t slot = 1; for (const Product& item : getProducts()) { if (item.id == id) break; ++slot; }
    if (!pulseDispenseSlot(slot, qty)) { sendError(request, 409, "machine_busy"); return; }
    user->credits -= qty * product->creditCost; user->lastCreditActivity = ""; addUserCredits(*user, 0); product->stock -= qty;
    if (!saveProducts() || !appendDispenseLog(user->id, product->id, qty)) { sendError(request, 500, "could_not_save_redemption"); return; }
    JsonDocument result; result["credits"] = user->credits; result["stock"] = product->stock; sendJson(request, 200, result);
}
static void handleAdminUsers(AsyncWebServerRequest* request) { User* user; if (!requireAdmin(request, user)) return; JsonDocument result; JsonArray list = result["users"].to<JsonArray>(); for (const User& item : getUsers()) userJson(list.add<JsonObject>(), item); sendJson(request, 200, result); }
static void handleAdminLog(AsyncWebServerRequest* request) { User* user; if (!requireAdmin(request, user)) return; JsonDocument doc; if (deserializeJson(doc, dispenseLogJson()) != DeserializationError::Ok) doc["log"].to<JsonArray>(); sendJson(request, 200, doc); }
static void handleAdminCredits(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) { User* user; if (!requireAdmin(request, user)) return; JsonDocument doc; bool ready; requireValidBody(request, data, len, index, total, doc, ready); if (!ready) return; User* target = findUserById(doc["user_id"] | ""); if (!target) { sendError(request, 404, "user_not_found"); return; } setUserCredits(*target, doc["credits"] | -1); JsonDocument result; result["success"] = true; sendJson(request, 200, result); }
void updateReverseVending() {
    uint32_t now = millis(); if (active.locked && (int32_t)(active.expiresAt - now) <= 0) { active = {}; setBottleCollectionEnabled(false); }
    if (active.locked && consumeValidatedBottle()) { User* user = findUserById(active.userId); if (user) addUserCredits(*user, CREDIT_PER_BOTTLE); active.expiresAt = now + 60000; }
    if (now - lastExpirySweep > 180000) { lastExpirySweep = now; expireInactiveCredits(); }
}
void setupAPI(AsyncWebServer& server) {
    server.on("/api/register", HTTP_POST, [](AsyncWebServerRequest*) {}, nullptr, handleRegister); server.on("/api/login", HTTP_POST, [](AsyncWebServerRequest*) {}, nullptr, handleLogin);
    server.on("/api/logout", HTTP_POST, handleLogout); server.on("/api/me", HTTP_GET, handleMe); server.on("/api/user/password", HTTP_POST, [](AsyncWebServerRequest*) {}, nullptr, handlePassword);
    server.on("/api/products", HTTP_GET, handleProducts); server.on("/api/products", HTTP_POST, [](AsyncWebServerRequest*) {}, nullptr, handleAddProduct); server.on("/api/products", HTTP_PUT, [](AsyncWebServerRequest*) {}, nullptr, handleUpdateProduct); server.on("/api/products", HTTP_DELETE, handleDeleteProduct);
    server.on("/api/session/start", HTTP_POST, handleStart); server.on("/api/session/end", HTTP_POST, handleEnd); server.on("/api/session/status", HTTP_GET, handleStatus); server.on("/api/redeem", HTTP_POST, [](AsyncWebServerRequest*) {}, nullptr, handleRedeem);
    server.on("/api/admin/users", HTTP_GET, handleAdminUsers); server.on("/api/admin/log", HTTP_GET, handleAdminLog); server.on("/api/admin/credits", HTTP_POST, [](AsyncWebServerRequest*) {}, nullptr, handleAdminCredits);
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html"); server.onNotFound([](AsyncWebServerRequest* request) { request->send(404, "application/json", "{\"error\":\"not_found\"}"); });
    server.on("/portal", HTTP_GET, [](AsyncWebServerRequest* request) { request->send(LittleFS, "/portal.html", "text/html"); });
    server.on("/admin", HTTP_GET, [](AsyncWebServerRequest* request) { request->send(LittleFS, "/admin.html", "text/html"); });
}
