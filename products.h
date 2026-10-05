#pragma once
#include <Arduino.h>
#include <vector>

struct Product { String id, name; int creditCost = 0, stock = 0; };
bool loadProducts();
bool saveProducts();
const std::vector<Product>& getProducts();
Product* getProductById(const String& id);
bool addProduct(const String& name, int creditCost, int stock, String& id);
bool updateProduct(const String& id, const String& name, int creditCost, int stock);
bool deleteProduct(const String& id);
