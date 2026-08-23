#pragma once
// ============================================================
//  products.h — Product data model and CRUD declarations
// ============================================================
#include <Arduino.h>
#include <vector>

struct Product {
    uint32_t id;
    String   name;
    int32_t  stocks;
    float    price;
};

/**
 * Load products from LittleFS (/products.json).
 * Returns true if the file was read and parsed successfully.
 * Returns true (with empty list) if the file does not yet exist.
 */
bool loadProducts();

/**
 * Persist the current product list to LittleFS.
 * Returns true on success.
 */
bool saveProducts();

/** Return a const reference to the in-RAM product list. */
const std::vector<Product>& getProducts();

/**
 * Find a product by ID.
 * Returns a pointer to the product or nullptr if not found.
 */
Product* getProductById(uint32_t id);

/**
 * Add a new product.  Assigns a unique auto-increment ID.
 * Returns the newly created Product.
 */
Product addProduct(const String& name, int32_t stocks, float price);

/**
 * Update an existing product by ID.
 * Returns true if found and updated.
 */
bool updateProduct(uint32_t id, const String& name, int32_t stocks, float price);

/**
 * Delete a product by ID.
 * Returns true if found and deleted.
 */
bool deleteProduct(uint32_t id);

/** Compute total inventory value across all products. */
float getTotalValue();

/** Count products with stocks <= threshold (default 10). */
int  getLowStockCount(int32_t threshold = 10);
