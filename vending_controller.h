#pragma once
#include <Arduino.h>

enum class DispenseResult : uint8_t { None, Delivered, TimedOut };
bool initVendingMachine();
void updateVendingMachine();
void setBottleCollectionEnabled(bool enabled);
void setBottleStorageFull(bool full);
bool consumeValidatedBottle(int& credits);
bool bottleDetectionPending();
int finishBottleCollection();
bool startBottleDoorCycle();
uint32_t bottleDoorSecondsRemaining();
bool startDispense(uint8_t slot);
DispenseResult takeDispenseResult();
uint8_t takeButtonPress();
uint8_t blockedDropSensorMask();
