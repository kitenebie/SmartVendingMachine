#include "products.h"
#include "storage.h"
#include <ArduinoJson.h>
#include <esp_random.h>

static const char* PRODUCTS_FILE = "/data/products.json";
static std::vector<Product> products;

static String productId() {
    char value[12];
    snprintf(value, sizeof(value), "p%08lx", (unsigned long)esp_random());
    return value;
}

static bool validProducts(const std::vector<Product>& list) {
    bool occupied[5] = {};
    if (list.size() > 4) return false;
    for (const Product& product : list) {
        if (product.id.isEmpty() || product.name.isEmpty() || product.creditCost < 0 ||
            product.stock < 0 || product.slot < 1 || product.slot > 4 || occupied[product.slot]) return false;
        occupied[product.slot] = true;
    }
    return true;
}

bool saveProducts() {
    JsonDocument doc;
    JsonArray items = doc["products"].to<JsonArray>();
    for (const Product& product : products) {
        JsonObject item = items.add<JsonObject>();
        item["id"] = product.id;
        item["name"] = product.name;
        item["credit_cost"] = product.creditCost;
        item["stock"] = product.stock;
        item["slot"] = product.slot;
    }
    String out;
    serializeJson(doc, out);
    return writeFile(PRODUCTS_FILE, out);
}

bool loadProducts() {
    products.clear();
    String json = readFile(PRODUCTS_FILE);
    bool legacy = false;
    if (json.isEmpty() && !fileExists(PRODUCTS_FILE)) {
        json = readFile("/products.json");
        legacy = !json.isEmpty();
    }
    if (json.isEmpty()) {
        if (fileExists(PRODUCTS_FILE)) return false;
        products = {{productId(), "Rice 1kg", 10, 20, 1},
                    {productId(), "Juice", 5, 20, 2},
                    {productId(), "Water", 3, 20, 3},
                    {productId(), "Snack", 8, 20, 4}};
        return saveProducts();
    }
    JsonDocument doc;
    if (deserializeJson(doc, json) != DeserializationError::Ok) return false;
    JsonArray items = doc["products"].is<JsonArray>() ? doc["products"].as<JsonArray>() : doc.as<JsonArray>();
    if (items.isNull() || items.size() > 4) return false;
    bool migrated = legacy;
    for (JsonObject item : items) {
        Product product;
        product.id = item["id"].is<const char*>() ? String(item["id"].as<const char*>()) : "p" + String(item["id"] | 0);
        product.name = item["name"] | "";
        product.creditCost = item["credit_cost"] | (int)(item["price"] | 0);
        product.stock = item["stock"] | (int)(item["stocks"] | 0);
        product.slot = item["slot"] | 0;
        if (!product.slot) { product.slot = products.size() + 1; migrated = true; }
        products.push_back(product);
    }
    if (!validProducts(products)) return false;
    return !migrated || saveProducts();
}

const std::vector<Product>& getProducts() { return products; }
Product* getProductById(const String& id) {
    for (Product& product : products) if (product.id == id) return &product;
    return nullptr;
}
bool addProduct(const String& name, int creditCost, int stock, String& id) {
    if (name.isEmpty() || creditCost < 0 || stock < 0 || products.size() >= 4) return false;
    bool occupied[5] = {};
    for (const Product& product : products) occupied[product.slot] = true;
    uint8_t slot = 1;
    while (occupied[slot]) ++slot;
    do { id = productId(); } while (getProductById(id));
    products.push_back({id, name, creditCost, stock, slot});
    if (saveProducts()) return true;
    products.pop_back();
    return false;
}
bool updateProduct(const String& id, const String& name, int creditCost, int stock) {
    Product* product = getProductById(id);
    if (!product || name.isEmpty() || creditCost < 0 || stock < 0) return false;
    Product previous = *product;
    product->name = name; product->creditCost = creditCost; product->stock = stock;
    if (saveProducts()) return true;
    *product = previous;
    return false;
}
bool deleteProduct(const String& id) {
    for (auto it = products.begin(); it != products.end(); ++it) if (it->id == id) {
        Product removed = *it;
        size_t position = it - products.begin();
        products.erase(it);
        if (saveProducts()) return true;
        products.insert(products.begin() + position, removed);
        return false;
    }
    return false;
}
