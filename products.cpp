#include "products.h"
// Guide: Ito ang product inventory sa memory at ang CRUD helpers nito.
// Sa boot, loadProducts() ang nagbabasa ng saved JSON (o legacy/default data);
// bawat add, update at delete ay sine-save ulit sa LittleFS.
#include "storage.h"
#include <ArduinoJson.h>
#include <esp_random.h>

static const char* PRODUCTS_FILE = "/data/products.json";
static std::vector<Product> products;
static String productId() { char value[12]; snprintf(value, sizeof(value), "p%08lx", (unsigned long)esp_random()); return value; }
bool saveProducts() {
    JsonDocument doc; JsonArray items = doc["products"].to<JsonArray>();
    for (const Product& product : products) { JsonObject item = items.add<JsonObject>(); item["id"] = product.id; item["name"] = product.name; item["credit_cost"] = product.creditCost; item["stock"] = product.stock; }
    String out; serializeJson(doc, out); return writeFile(PRODUCTS_FILE, out);
}
bool loadProducts() {
    products.clear(); String json = readFile(PRODUCTS_FILE); if (json.isEmpty()) json = readFile("/products.json");
    if (json.isEmpty()) { String id; addProduct("Rice 1kg", 10, 20, id); addProduct("Juice", 5, 20, id); addProduct("Water", 3, 20, id); addProduct("Snack", 8, 20, id); return true; }
    JsonDocument doc; if (deserializeJson(doc, json) != DeserializationError::Ok) return false;
    JsonArray items = doc["products"].is<JsonArray>() ? doc["products"].as<JsonArray>() : doc.as<JsonArray>();
    for (JsonObject item : items) { Product product; product.id = item["id"].is<const char*>() ? String(item["id"].as<const char*>()) : "p" + String(item["id"] | 0); product.name = item["name"] | ""; product.creditCost = item["credit_cost"] | (int)(item["price"] | 0); product.stock = item["stock"] | (int)(item["stocks"] | 0); if (!product.id.isEmpty() && !product.name.isEmpty()) products.push_back(product); }
    return true;
}
const std::vector<Product>& getProducts() { return products; }
Product* getProductById(const String& id) { for (Product& product : products) if (product.id == id) return &product; return nullptr; }
bool addProduct(const String& name, int creditCost, int stock, String& id) { if (name.isEmpty() || creditCost < 0 || stock < 0) return false; id = productId(); products.push_back({id, name, creditCost, stock}); return saveProducts(); }
bool updateProduct(const String& id, const String& name, int creditCost, int stock) { Product* product = getProductById(id); if (!product || name.isEmpty() || creditCost < 0 || stock < 0) return false; product->name = name; product->creditCost = creditCost; product->stock = stock; return saveProducts(); }
bool deleteProduct(const String& id) { for (auto it = products.begin(); it != products.end(); ++it) if (it->id == id) { products.erase(it); return saveProducts(); } return false; }
