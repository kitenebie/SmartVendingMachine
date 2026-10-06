#include "vending_controller.h"
// Taglish Guide: Dito tumatakbo ang hardware state machine. updateVendingMachine()
// ang nagbabasa ng bottle sensors at nagkokontrol ng door/dispense servos;
// API naman ang tumatawag sa bottle-credit helpers kapag valid ang bote.
#include "config.h"
#include "pin_config.h"
#include "display_manager.h"

static bool collectionEnabled = false, bottleInFlight = false, bottleReady = false, doorOpen = false;
static uint8_t dispensingSlot = 0, dispensingRemaining = 0;
static uint32_t validationStarted = 0, doorOpenedAt = 0, dispenseUntil = 0;
static const uint32_t SERVO_STOP = 4915, SERVO_PUSH = 6553, SERVO_MIN = 3277, SERVO_MAX = 6553;
static uint32_t angleDuty(int angle) { return SERVO_MIN + ((SERVO_MAX - SERVO_MIN) * constrain(angle, 0, 180)) / 180; }
static void door(bool open) { if (doorOpen == open) return; ledcWrite(getPinConfig().doorServo, angleDuty(open ? DOOR_OPEN_ANGLE : DOOR_CLOSED_ANGLE)); doorOpen = open; if (open) doorOpenedAt = millis(); }
static void stopDispensers() { const PinConfig& p = getPinConfig(); ledcWrite(p.servo1, SERVO_STOP); ledcWrite(p.servo2, SERVO_STOP); ledcWrite(p.servo3, SERVO_STOP); ledcWrite(p.servo4, SERVO_STOP); }
void initVendingMachine() {
    const PinConfig& p = getPinConfig(); pinMode(p.sensorEntry, INPUT); pinMode(p.sensorA, INPUT); pinMode(p.sensorB, INPUT);
    ledcAttach(p.servo1, 50, 16); ledcAttach(p.servo2, 50, 16); ledcAttach(p.servo3, 50, 16); ledcAttach(p.servo4, 50, 16); ledcAttach(p.doorServo, 50, 16);
    stopDispensers(); doorOpen = true; door(false); initDisplay(); displayCredits(0);
}
void setBottleCollectionEnabled(bool enabled) { collectionEnabled = enabled; if (!enabled) { validationStarted = 0; bottleInFlight = false; bottleReady = false; door(false); } }
bool consumeValidatedBottle() { bool result = bottleReady; bottleReady = false; return result; }
bool pulseDispenseSlot(uint8_t slot, uint8_t count) {
    if (slot < 1 || slot > 4 || count < 1 || dispenseUntil > millis()) return false; const PinConfig& p = getPinConfig(); dispensingSlot = slot; dispensingRemaining = count;
    if (slot == 1) ledcWrite(p.servo1, SERVO_PUSH); else if (slot == 2) ledcWrite(p.servo2, SERVO_PUSH); else if (slot == 3) ledcWrite(p.servo3, SERVO_PUSH); else ledcWrite(p.servo4, SERVO_PUSH);
    dispenseUntil = millis() + 750; return true;
}
void updateVendingMachine() {
    uint32_t now = millis(); const PinConfig& p = getPinConfig(); if (dispenseUntil && now >= dispenseUntil) { stopDispensers(); if (dispensingRemaining > 1) { --dispensingRemaining; pulseDispenseSlot(dispensingSlot, dispensingRemaining); } else { dispensingRemaining = 0; dispensingSlot = 0; dispenseUntil = 0; } } if (!collectionEnabled) return;
    bool entry = digitalRead(p.sensorEntry) == HIGH, sensorA = digitalRead(p.sensorA) == HIGH, sensorB = digitalRead(p.sensorB) == HIGH;
    if (!bottleInFlight && !entry && sensorA && sensorB) { if (!validationStarted) validationStarted = now; else if (now - validationStarted >= BOTTLE_VALIDATION_TIME_MS) { bottleInFlight = true; bottleReady = true; validationStarted = 0; door(true); } }
    else if (!bottleInFlight) validationStarted = 0;
    if (bottleInFlight && !entry && !sensorA && !sensorB && now - doorOpenedAt >= DOOR_MIN_OPEN_TIME_MS) { bottleInFlight = false; door(false); }
}
