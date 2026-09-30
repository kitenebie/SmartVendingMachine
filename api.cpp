// ============================================================
//  api.cpp -- REST API implementation
//  Includes Authentication, Products CRUD, Vending & Pin Config APIs
// ============================================================
#include "api.h"
#include "auth.h"
#include "products.h"
#include "pin_config.h"
#include "vending_controller.h"
#include <ArduinoJson.h>
#include <LittleFS.h>

// ---- Helpers -----------------------------------------------

static void sendJson(AsyncWebServerRequest* req, int code, const String& body) {
    AsyncWebServerResponse* resp =
        req->beginResponse(code, "application/json", body);
    resp->addHeader("Access-Control-Allow-Origin", "*");
    resp->addHeader("Cache-Control", "no-cache");
    req->send(resp);
}

static void sendOk(AsyncWebServerRequest* req, const String& msg = "OK") {
    sendJson(req, 200, "{\"success\":true,\"message\":\"" + msg + "\"}");
}

static void sendError(AsyncWebServerRequest* req, int code, const String& msg) {
    sendJson(req, code, "{\"success\":false,\"message\":\"" + msg + "\"}");
}

String extractToken(AsyncWebServerRequest* req) {
    if (req->hasHeader("X-Session-Token")) {
        return req->getHeader("X-Session-Token")->value();
    }
    if (req->hasParam("token")) {
        return req->getParam("token")->value();
    }
    return "";
}

bool isAuthenticated(AsyncWebServerRequest* req) {
    return validateSession(extractToken(req));
}

// ---- OPTIONS pre-flight ------------------------------------

static void handleOptions(AsyncWebServerRequest* req) {
    AsyncWebServerResponse* resp = req->beginResponse(204);
    resp->addHeader("Access-Control-Allow-Origin",  "*");
    resp->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
    resp->addHeader("Access-Control-Allow-Headers", "Content-Type,X-Session-Token");
    req->send(resp);
}

// ============================================================
//  AUTH ENDPOINTS
// ============================================================

static void handleLogin(AsyncWebServerRequest* req, uint8_t* data,
                        size_t len, size_t, size_t) {
    String body = String((char*)data, len);
    JsonDocument doc;
    if (deserializeJson(doc, body) != DeserializationError::Ok) {
        sendError(req, 400, "Invalid JSON");
        return;
    }
    String username = doc["username"] | "";
    String password = doc["password"] | "";

    if (username.isEmpty() || password.isEmpty()) {
        sendError(req, 400, "Username and password are required");
        return;
    }

    if (!validateCredentials(username, password)) {
        sendError(req, 401, "Invalid username or password");
        return;
    }

    String token = createSession();
    String resp  = "{\"success\":true,\"message\":\"Login successful\",\"token\":\"" +
                   token + "\"}";
    sendJson(req, 200, resp);
}

static void handleLogout(AsyncWebServerRequest* req, uint8_t* data,
                         size_t len, size_t, size_t) {
    String token = extractToken(req);
    if (!token.isEmpty()) destroySession(token);
    sendOk(req, "Logged out");
}

static void handleSession(AsyncWebServerRequest* req) {
    if (!isAuthenticated(req)) {
        sendError(req, 401, "Unauthorized");
        return;
    }
    String user = getStoredUsername();
    sendJson(req, 200, "{\"success\":true,\"username\":\"" + user + "\"}");
}

// ============================================================
//  PRODUCT & VENDING ENDPOINTS
// ============================================================

static void handleGetProducts(AsyncWebServerRequest* req) {
    if (!isAuthenticated(req)) { sendError(req, 401, "Unauthorized"); return; }

    const auto& products = getProducts();
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    for (const auto& p : products) {
        JsonObject obj = arr.add<JsonObject>();
        obj["id"]         = p.id;
        obj["name"]       = p.name;
        obj["stocks"]     = p.stocks;
        obj["price"]      = p.price;
        obj["totalValue"] = (float)p.stocks * p.price;
    }

    String out;
    serializeJson(doc, out);

    int32_t totalStocks = 0;
    for (const auto& p : products) totalStocks += p.stocks;

    String resp = "{\"success\":true,\"products\":" + out +
                  ",\"summary\":{" +
                  "\"totalProducts\":" + String(products.size()) +
                  ",\"totalStocks\":"  + String(totalStocks) +
                  ",\"totalValue\":"   + String(getTotalValue(), 2) +
                  ",\"lowStock\":"     + String(getLowStockCount()) +
                  ",\"machineCredits\":" + String(getCurrentCredits()) +
                  ",\"machineState\":\"" + getVendingStateName() + "\"" +
                  "}}";
    sendJson(req, 200, resp);
}

static void handleAddProduct(AsyncWebServerRequest* req, uint8_t* data,
                             size_t len, size_t, size_t) {
    if (!isAuthenticated(req)) { sendError(req, 401, "Unauthorized"); return; }

    String body = String((char*)data, len);
    JsonDocument doc;
    if (deserializeJson(doc, body) != DeserializationError::Ok) {
        sendError(req, 400, "Invalid JSON"); return;
    }

    String name   = doc["name"]   | "";
    int32_t stocks = doc["stocks"] | -1;
    float price   = doc["price"]  | -1.0f;

    name.trim();
    if (name.isEmpty())    { sendError(req, 400, "Product name is required"); return; }
    if (stocks < 0)        { sendError(req, 400, "Stocks must be >= 0"); return; }
    if (price < 0)         { sendError(req, 400, "Price must be >= 0"); return; }

    Product p = addProduct(name, stocks, price);

    String resp = "{\"success\":true,\"message\":\"Product added\","
                  "\"product\":{\"id\":" + String(p.id) +
                  ",\"name\":\"" + p.name + "\""
                  ",\"stocks\":" + String(p.stocks) +
                  ",\"price\":" + String(p.price, 2) + "}}";
    sendJson(req, 201, resp);
}

static void handleGetProduct(AsyncWebServerRequest* req) {
    if (!isAuthenticated(req)) { sendError(req, 401, "Unauthorized"); return; }
    if (!req->hasParam("id")) { sendError(req, 400, "Missing id"); return; }

    uint32_t id = (uint32_t)req->getParam("id")->value().toInt();
    Product* p  = getProductById(id);
    if (!p) { sendError(req, 404, "Product not found"); return; }

    String resp = "{\"success\":true,\"product\":{\"id\":" + String(p->id) +
                  ",\"name\":\"" + p->name + "\""
                  ",\"stocks\":" + String(p->stocks) +
                  ",\"price\":" + String(p->price, 2) + "}}";
    sendJson(req, 200, resp);
}

static void handleUpdateProduct(AsyncWebServerRequest* req, uint8_t* data,
                                size_t len, size_t, size_t) {
    if (!isAuthenticated(req)) { sendError(req, 401, "Unauthorized"); return; }
    if (!req->hasParam("id")) { sendError(req, 400, "Missing id"); return; }

    uint32_t id = (uint32_t)req->getParam("id")->value().toInt();
    String body = String((char*)data, len);
    JsonDocument doc;
    if (deserializeJson(doc, body) != DeserializationError::Ok) {
        sendError(req, 400, "Invalid JSON"); return;
    }

    String name    = doc["name"]   | "";
    int32_t stocks = doc["stocks"] | -1;
    float price    = doc["price"]  | -1.0f;

    name.trim();
    if (name.isEmpty()) { sendError(req, 400, "Product name is required"); return; }
    if (stocks < 0)     { sendError(req, 400, "Stocks must be >= 0"); return; }
    if (price < 0)      { sendError(req, 400, "Price must be >= 0"); return; }

    if (!updateProduct(id, name, stocks, price)) {
        sendError(req, 404, "Product not found"); return;
    }
    sendOk(req, "Product updated");
}

static void handleDeleteProduct(AsyncWebServerRequest* req) {
    if (!isAuthenticated(req)) { sendError(req, 401, "Unauthorized"); return; }
    if (!req->hasParam("id")) { sendError(req, 400, "Missing id"); return; }

    uint32_t id = (uint32_t)req->getParam("id")->value().toInt();
    if (!deleteProduct(id)) {
        sendError(req, 404, "Product not found"); return;
    }
    sendOk(req, "Product deleted");
}

// Remote Dispense API
static void handleRemoteDispense(AsyncWebServerRequest* req, uint8_t* data,
                                 size_t len, size_t, size_t) {
    if (!isAuthenticated(req)) { sendError(req, 401, "Unauthorized"); return; }

    String body = String((char*)data, len);
    JsonDocument doc;
    if (deserializeJson(doc, body) != DeserializationError::Ok) {
        sendError(req, 400, "Invalid JSON"); return;
    }

    int productId = doc["productId"] | 1;
    if (triggerDispense(productId)) {
        sendOk(req, "Dispense triggered for Product " + String(productId));
    } else {
        sendError(req, 409, "Machine is currently busy dispensing");
    }
}

// ============================================================
//  GPIO PIN CONFIGURATION ENDPOINTS (20x4 I2C LCD)
// ============================================================

static void handleGetPins(AsyncWebServerRequest* req) {
    if (!isAuthenticated(req)) { sendError(req, 401, "Unauthorized"); return; }

    const PinConfig& p = getPinConfig();
    JsonDocument doc;
    doc["success"] = true;
    JsonObject pins = doc["pins"].to<JsonObject>();
    pins["sensorEntry"] = p.sensorEntry;
    pins["sensorA"] = p.sensorA;
    pins["sensorB"] = p.sensorB;
    pins["btn1"]    = p.btn1;
    pins["btn2"]    = p.btn2;
    pins["btn3"]    = p.btn3;
    pins["btn4"]    = p.btn4;
    pins["ir1"]     = p.ir1;
    pins["ir2"]     = p.ir2;
    pins["ir3"]     = p.ir3;
    pins["ir4"]     = p.ir4;
    pins["servo1"]  = p.servo1;
    pins["servo2"]  = p.servo2;
    pins["servo3"]  = p.servo3;
    pins["servo4"]  = p.servo4;
    pins["doorServo"] = p.doorServo;
    pins["lcdSda"]  = p.lcdSda;
    pins["lcdScl"]  = p.lcdScl;

    String out;
    serializeJson(doc, out);
    sendJson(req, 200, out);
}

static void handleUpdatePins(AsyncWebServerRequest* req, uint8_t* data,
                             size_t len, size_t, size_t) {
    if (!isAuthenticated(req)) { sendError(req, 401, "Unauthorized"); return; }

    String body = String((char*)data, len);
    JsonDocument doc;
    if (deserializeJson(doc, body) != DeserializationError::Ok) {
        sendError(req, 400, "Invalid JSON"); return;
    }

    PinConfig current = getPinConfig();
    PinConfig newPins;

    newPins.sensorEntry = doc["sensorEntry"] | current.sensorEntry;
    newPins.sensorA = doc["sensorA"] | current.sensorA;
    newPins.sensorB = doc["sensorB"] | current.sensorB;
    newPins.btn1    = doc["btn1"]    | current.btn1;
    newPins.btn2    = doc["btn2"]    | current.btn2;
    newPins.btn3    = doc["btn3"]    | current.btn3;
    newPins.btn4    = doc["btn4"]    | current.btn4;
    newPins.ir1     = doc["ir1"]     | current.ir1;
    newPins.ir2     = doc["ir2"]     | current.ir2;
    newPins.ir3     = doc["ir3"]     | current.ir3;
    newPins.ir4     = doc["ir4"]     | current.ir4;
    newPins.servo1  = doc["servo1"]  | current.servo1;
    newPins.servo2  = doc["servo2"]  | current.servo2;
    newPins.servo3  = doc["servo3"]  | current.servo3;
    newPins.servo4  = doc["servo4"]  | current.servo4;
    newPins.doorServo = doc["doorServo"] | current.doorServo;
    newPins.lcdSda  = doc["lcdSda"]  | (doc["dispDio"] | current.lcdSda);
    newPins.lcdScl  = doc["lcdScl"]  | (doc["dispClk"] | current.lcdScl);

    // Validate outputs (servos & LCD I2C must NOT be input-only pins like 34-39)
    if (!isValidGpio(newPins.servo1, false) || !isValidGpio(newPins.servo2, false) ||
        !isValidGpio(newPins.servo3, false) || !isValidGpio(newPins.servo4, false) ||
        !isValidGpio(newPins.doorServo, false) ||
        !isValidGpio(newPins.lcdSda, false) || !isValidGpio(newPins.lcdScl, false)) {
        sendError(req, 400, "Servos and I2C LCD must use output-capable GPIOs (do not use 34, 35, 36, 39)");
        return;
    }

    if (savePinConfig(newPins)) {
        initVendingMachine(); // Re-apply pins immediately to hardware
        sendOk(req, "GPIO pins updated and 20x4 LCD reinitialized live!");
    } else {
        sendError(req, 500, "Failed to save pin configuration to storage");
    }
}

// ============================================================
//  CONFIGURATION ENDPOINTS (ACCOUNT)
// ============================================================

static void handleGetConfig(AsyncWebServerRequest* req) {
    if (!isAuthenticated(req)) { sendError(req, 401, "Unauthorized"); return; }
    String user = getStoredUsername();
    sendJson(req, 200, "{\"success\":true,\"username\":\"" + user + "\"}");
}

static void handleUpdateConfig(AsyncWebServerRequest* req, uint8_t* data,
                               size_t len, size_t, size_t) {
    if (!isAuthenticated(req)) { sendError(req, 401, "Unauthorized"); return; }

    String body = String((char*)data, len);
    JsonDocument doc;
    if (deserializeJson(doc, body) != DeserializationError::Ok) {
        sendError(req, 400, "Invalid JSON"); return;
    }

    String currentPassword = doc["currentPassword"] | "";
    String newUsername     = doc["newUsername"]     | "";
    String newPassword     = doc["newPassword"]     | "";
    String confirmPassword = doc["confirmPassword"] | "";

    String currentUser = getStoredUsername();
    if (!validateCredentials(currentUser, currentPassword)) {
        sendError(req, 401, "Current password is incorrect"); return;
    }

    newUsername.trim();
    if (newUsername.isEmpty()) { sendError(req, 400, "New username is required"); return; }
    if (newPassword.isEmpty()) { sendError(req, 400, "New password is required"); return; }
    if (newPassword != confirmPassword) {
        sendError(req, 400, "Passwords do not match"); return;
    }
    if (newPassword.length() < 6) {
        sendError(req, 400, "Password must be at least 6 characters"); return;
    }

    if (!updateCredentials(newUsername, newPassword)) {
        sendError(req, 500, "Failed to save credentials"); return;
    }

    destroyAllSessions();
    sendOk(req, "Credentials updated. Please log in again.");
}

// ============================================================
//  REGISTER ALL ROUTES
// ============================================================

void setupAPI(AsyncWebServer& server) {
    server.on("/*", HTTP_OPTIONS, handleOptions);

    // Auth
    server.on("/api/login",   HTTP_POST, [](AsyncWebServerRequest* r){}, nullptr, handleLogin);
    server.on("/api/logout",  HTTP_POST, [](AsyncWebServerRequest* r){}, nullptr, handleLogout);
    server.on("/api/session", HTTP_GET,  handleSession);

    // Products & Vending
    server.on("/api/products", HTTP_GET,  handleGetProducts);
    server.on("/api/products", HTTP_POST, [](AsyncWebServerRequest* r){}, nullptr, handleAddProduct);
    server.on("/api/product", HTTP_GET,    handleGetProduct);
    server.on("/api/product", HTTP_DELETE, handleDeleteProduct);
    server.on("/api/product", HTTP_PUT,    [](AsyncWebServerRequest* r){}, nullptr, handleUpdateProduct);

    // Remote Dispense
    server.on("/api/vending/dispense", HTTP_POST, [](AsyncWebServerRequest* r){}, nullptr, handleRemoteDispense);

    // Pin Configuration
    server.on("/api/pins", HTTP_GET, handleGetPins);
    server.on("/api/pins", HTTP_PUT, [](AsyncWebServerRequest* r){}, nullptr, handleUpdatePins);

    // Account Configuration
    server.on("/api/configuration", HTTP_GET, handleGetConfig);
    server.on("/api/configuration", HTTP_PUT, [](AsyncWebServerRequest* r){}, nullptr, handleUpdateConfig);

    // Static Web UI Files from LittleFS
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    server.onNotFound([](AsyncWebServerRequest* req) {
        if (req->method() == HTTP_OPTIONS) { handleOptions(req); return; }
        req->send(404, "text/plain", "Not found");
    });

    Serial.println("[API] Routes registered with Pin Configuration endpoints.");
}
