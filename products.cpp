// ============================================================
//  products.cpp -- Product CRUD + LittleFS persistence
// ============================================================
#include "products.h"
#include "storage.h"
#include <ArduinoJson.h>

static const char* PRODUCTS_FILE = "/products.json";

// In-RAM product list
static std::vector<Product> s_products;
// Auto-increment counter -- always larger than any existing ID
static uint32_t s_nextId = 1;

// ---- Helpers ------------------------------------------------

static void recalcNextId() {
    s_nextId = 1;
    for (const auto& p : s_products) {
        if (p.id >= s_nextId) s_nextId = p.id + 1;
    }
}

// Seed 4 default products if running for the first time
static void seedDefaultProducts() {
    s_products.clear();
    s_products.push_back({1, "Product 1 (Water)", 10, 2.0f});
    s_products.push_back({2, "Product 2 (Juice)", 10, 3.0f});
    s_products.push_back({3, "Product 3 (Soda)",  10, 4.0f});
    s_products.push_back({4, "Product 4 (Snack)", 10, 2.0f});
    s_nextId = 5;
    saveProducts();
    Serial.println("[Products] Initialized 4 default products in LittleFS database.");
}

// ---- Public API ---------------------------------------------

bool loadProducts() {
    s_products.clear();

    if (!fileExists(PRODUCTS_FILE)) {
        Serial.println("[Products] No product file found, seeding default 4 products...");
        seedDefaultProducts();
        return true;
    }

    String json = readFile(PRODUCTS_FILE);
    if (json.isEmpty()) {
        Serial.println("[Products] Product file empty, seeding defaults...");
        seedDefaultProducts();
        return true;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        Serial.printf("[Products] JSON parse error: %s\n", err.c_str());
        return false;
    }

    JsonArray arr = doc.as<JsonArray>();
    for (JsonObject obj : arr) {
        Product p;
        p.id     = obj["id"]     | 0u;
        p.name   = obj["name"]   | "";
        p.stocks = obj["stocks"] | 0;
        p.price  = obj["price"]  | 0.0f;
        if (p.id > 0 && p.name.length() > 0) {
            s_products.push_back(p);
        }
    }

    recalcNextId();
    Serial.printf("[Products] Loaded %d products\n", (int)s_products.size());
    return true;
}

bool saveProducts() {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    for (const auto& p : s_products) {
        JsonObject obj = arr.add<JsonObject>();
        obj["id"]     = p.id;
        obj["name"]   = p.name;
        obj["stocks"] = p.stocks;
        obj["price"]  = p.price;
    }

    String out;
    serializeJson(doc, out);
    bool ok = writeFile(PRODUCTS_FILE, out);
    if (!ok) Serial.println("[Products] ERROR: Failed to save products!");
    return ok;
}

const std::vector<Product>& getProducts() {
    return s_products;
}

Product* getProductById(uint32_t id) {
    for (auto& p : s_products) {
        if (p.id == id) return &p;
    }
    return nullptr;
}

Product addProduct(const String& name, int32_t stocks, float price) {
    Product p;
    p.id     = s_nextId++;
    p.name   = name;
    p.stocks = stocks;
    p.price  = price;
    s_products.push_back(p);
    saveProducts();
    Serial.printf("[Products] Added: id=%u name=%s\n", p.id, p.name.c_str());
    return p;
}

bool updateProduct(uint32_t id, const String& name, int32_t stocks, float price) {
    Product* p = getProductById(id);
    if (!p) return false;
    p->name   = name;
    p->stocks = stocks;
    p->price  = price;
    saveProducts();
    Serial.printf("[Products] Updated id=%u\n", id);
    return true;
}

bool deleteProduct(uint32_t id) {
    for (auto it = s_products.begin(); it != s_products.end(); ++it) {
        if (it->id == id) {
            s_products.erase(it);
            saveProducts();
            Serial.printf("[Products] Deleted id=%u\n", id);
            return true;
        }
    }
    return false;
}

float getTotalValue() {
    float total = 0.0f;
    for (const auto& p : s_products) {
        total += (float)p.stocks * p.price;
    }
    return total;
}

int getLowStockCount(int32_t threshold) {
    int count = 0;
    for (const auto& p : s_products) {
        if (p.stocks <= threshold) count++;
    }
    return count;
}
