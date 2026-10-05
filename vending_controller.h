#pragma once
#include <Arduino.h>

void initVendingMachine();
void updateVendingMachine();
void setBottleCollectionEnabled(bool enabled);
bool consumeValidatedBottle();
bool pulseDispenseSlot(uint8_t slot, uint8_t count = 1);
