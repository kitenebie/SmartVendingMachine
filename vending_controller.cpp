#include "vending_controller.h"
#include "config.h"
#include "pin_config.h"
#include "display_manager.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

static SemaphoreHandle_t hardwareMutex;
static bool collectionEnabled = false, bottleInFlight = false, bottleReady = false, doorOpen = false;
// Wait for all four sensors to clear before accepting a bottle.
static bool collectionArmed = false;
static bool bottleStorageFull = false;
static uint32_t validationStarted = 0, dispenseStartedAt = 0, buzzerOffAt = 0;
static int bottleCredits = 0;
static uint8_t dispensingSlot = 0, buttonPress = 0;
static uint8_t buttonRaw[4], buttonStable[4];
static uint32_t buttonChangedAt[4];
static DispenseResult dispenseResult = DispenseResult::None;
static const uint32_t SERVO_MIN = 3277, SERVO_MAX = 6553;
static constexpr int bottleCreditsForSensors(bool sensorA, bool sensorB, bool sensorC, bool sensorD) {
    return !sensorA || !sensorB || (!sensorC && sensorD) ? 0 : sensorD ? 5 : sensorC ? 3 : 1;
}
static_assert(bottleCreditsForSensors(true, true, false, false) == 1 &&
              bottleCreditsForSensors(true, true, true, false) == 3 &&
              bottleCreditsForSensors(true, true, true, true) == 5 &&
              bottleCreditsForSensors(true, true, false, true) == 0 &&
              bottleCreditsForSensors(false, true, true, true) == 0 &&
              bottleCreditsForSensors(true, false, true, true) == 0, "Bottle credit tiers changed");

static int pwmChannel(int pin) {
    const PinConfig& p = getPinConfig();
    if (pin == p.servo1) return 0;
    if (pin == p.servo2) return 1;
    if (pin == p.servo3) return 2;
    if (pin == p.servo4) return 3;
    return 4;
}
static bool attachPwm(int pin) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    return ledcAttach(pin, 50, 16);
#else
    int channel = pwmChannel(pin);
    return ledcSetup(channel, 50, 16) > 0 && (ledcAttachPin(pin, channel), true);
#endif
}
static void writePwm(int pin, uint32_t duty) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(pin, duty);
#else
    ledcWrite(pwmChannel(pin), duty);
#endif
}

static uint32_t angleDuty(int angle) {
    return SERVO_MIN + ((SERVO_MAX - SERVO_MIN) * constrain(angle, 0, 180)) / 180;
}
static void door(bool open) {
    if (doorOpen == open) return;
    writePwm(getPinConfig().doorServo, angleDuty(open ? DOOR_OPEN_ANGLE : DOOR_CLOSED_ANGLE));
    doorOpen = open;
}
static void stopDispensers() {
    const PinConfig& p = getPinConfig();
    writePwm(p.servo1, SERVO_STOP_DUTY);
    writePwm(p.servo2, SERVO_STOP_DUTY);
    writePwm(p.servo3, SERVO_STOP_DUTY);
    writePwm(p.servo4, SERVO_STOP_DUTY);
}
static bool anyDropDetected() {
    const PinConfig& p = getPinConfig();
    return digitalRead(p.ir1) == IR_ACTIVE_LEVEL || digitalRead(p.ir2) == IR_ACTIVE_LEVEL ||
           digitalRead(p.ir3) == IR_ACTIVE_LEVEL || digitalRead(p.ir4) == IR_ACTIVE_LEVEL;
}
uint8_t blockedDropSensorMask() {
    xSemaphoreTake(hardwareMutex, portMAX_DELAY);
    const PinConfig& p = getPinConfig();
    const int sensors[4] = {p.ir1, p.ir2, p.ir3, p.ir4};
    uint8_t mask = 0;
    for (int i = 0; i < 4; ++i)
        if (digitalRead(sensors[i]) == IR_ACTIVE_LEVEL) mask |= 1u << i;
    xSemaphoreGive(hardwareMutex);
    return mask;
}

bool initVendingMachine() {
    hardwareMutex = xSemaphoreCreateMutex();
    if (!hardwareMutex) return false;
    const PinConfig& p = getPinConfig();
    pinMode(BUZZER_PIN, OUTPUT); digitalWrite(BUZZER_PIN, LOW);
    pinMode(p.sensorA, INPUT); pinMode(p.sensorB, INPUT);
    pinMode(p.sensorC, INPUT); pinMode(p.sensorD, INPUT);
    const int buttons[4] = {p.btn1, p.btn2, p.btn3, p.btn4};
    const int sensors[4] = {p.ir1, p.ir2, p.ir3, p.ir4};
    for (int i = 0; i < 4; ++i) {
        pinMode(buttons[i], INPUT_PULLUP);
        pinMode(sensors[i], INPUT_PULLUP);
        buttonRaw[i] = buttonStable[i] = digitalRead(buttons[i]);
        buttonChangedAt[i] = millis();
    }
    if (!attachPwm(p.servo1) || !attachPwm(p.servo2) ||
        !attachPwm(p.servo3) || !attachPwm(p.servo4) ||
        !attachPwm(p.doorServo)) return false;
    stopDispensers();
    doorOpen = true; door(false);
    initDisplay();
    displayCredits(0);
    return true;
}

void setBottleCollectionEnabled(bool enabled) {
    xSemaphoreTake(hardwareMutex, portMAX_DELAY);
    collectionEnabled = enabled;
    if (enabled) {
        const PinConfig& p = getPinConfig();
        collectionArmed = digitalRead(p.sensorA) != BOTTLE_SENSOR_AB_ACTIVE_LEVEL &&
                          digitalRead(p.sensorB) != BOTTLE_SENSOR_AB_ACTIVE_LEVEL &&
                          digitalRead(p.sensorC) != BOTTLE_SENSOR_CD_ACTIVE_LEVEL &&
                          digitalRead(p.sensorD) != BOTTLE_SENSOR_CD_ACTIVE_LEVEL;
    } else {
        validationStarted = 0; bottleInFlight = false; bottleReady = false; bottleCredits = 0;
        collectionArmed = false;
        door(false);
    }
    xSemaphoreGive(hardwareMutex);
}
void setBottleStorageFull(bool full) {
    xSemaphoreTake(hardwareMutex, portMAX_DELAY);
    bottleStorageFull = full;
    if (full && !bottleInFlight) { validationStarted = 0; collectionArmed = false; door(false); }
    xSemaphoreGive(hardwareMutex);
}
void alertStorageFull() {
    xSemaphoreTake(hardwareMutex, portMAX_DELAY);
    digitalWrite(BUZZER_PIN, HIGH);
    buzzerOffAt = millis() + BUZZER_ALERT_MS;
    xSemaphoreGive(hardwareMutex);
}
bool consumeValidatedBottle(int& credits) {
    xSemaphoreTake(hardwareMutex, portMAX_DELAY);
    bool result = bottleReady;
    if (result) credits = bottleCredits;
    bottleReady = false;
    xSemaphoreGive(hardwareMutex);
    return result;
}
bool bottleDetectionPending() {
    xSemaphoreTake(hardwareMutex, portMAX_DELAY);
    bool pending = validationStarted || bottleInFlight || bottleReady;
    xSemaphoreGive(hardwareMutex);
    return pending;
}
bool startDispense(uint8_t slot) {
    xSemaphoreTake(hardwareMutex, portMAX_DELAY);
    bool ready = slot >= 1 && slot <= 4 && !dispensingSlot &&
                 dispenseResult == DispenseResult::None && !anyDropDetected();
    if (ready) {
        const PinConfig& p = getPinConfig();
        const int servos[4] = {p.servo1, p.servo2, p.servo3, p.servo4};
        dispensingSlot = slot;
        dispenseStartedAt = millis();
        writePwm(servos[slot - 1], SERVO_PUSH_DUTY);
    }
    xSemaphoreGive(hardwareMutex);
    return ready;
}
DispenseResult takeDispenseResult() {
    xSemaphoreTake(hardwareMutex, portMAX_DELAY);
    DispenseResult result = dispenseResult;
    dispenseResult = DispenseResult::None;
    xSemaphoreGive(hardwareMutex);
    return result;
}
uint8_t takeButtonPress() {
    xSemaphoreTake(hardwareMutex, portMAX_DELAY);
    uint8_t result = buttonPress;
    buttonPress = 0;
    xSemaphoreGive(hardwareMutex);
    return result;
}

void updateVendingMachine() {
    xSemaphoreTake(hardwareMutex, portMAX_DELAY);
    uint32_t now = millis();
    if (buzzerOffAt && (int32_t)(now - buzzerOffAt) >= 0) {
        digitalWrite(BUZZER_PIN, LOW);
        buzzerOffAt = 0;
    }
    const PinConfig& p = getPinConfig();
    const int buttons[4] = {p.btn1, p.btn2, p.btn3, p.btn4};
    for (int i = 0; i < 4; ++i) {
        uint8_t raw = digitalRead(buttons[i]);
        if (raw != buttonRaw[i]) { buttonRaw[i] = raw; buttonChangedAt[i] = now; }
        if (raw != buttonStable[i] && now - buttonChangedAt[i] >= BTN_DEBOUNCE_MS) {
            buttonStable[i] = raw;
            if (raw == LOW && !buttonPress) buttonPress = i + 1;
        }
    }
    if (dispensingSlot && (anyDropDetected() || now - dispenseStartedAt >= DISPENSE_TIMEOUT_MS)) {
        dispenseResult = anyDropDetected() ? DispenseResult::Delivered : DispenseResult::TimedOut;
        stopDispensers();
        dispensingSlot = 0;
    }
    if (collectionEnabled) {
        bool sensorA = digitalRead(p.sensorA) == BOTTLE_SENSOR_AB_ACTIVE_LEVEL;
        bool sensorB = digitalRead(p.sensorB) == BOTTLE_SENSOR_AB_ACTIVE_LEVEL;
        bool sensorC = digitalRead(p.sensorC) == BOTTLE_SENSOR_CD_ACTIVE_LEVEL;
        bool sensorD = digitalRead(p.sensorD) == BOTTLE_SENSOR_CD_ACTIVE_LEVEL;
        int detectedCredits = bottleCreditsForSensors(sensorA, sensorB, sensorC, sensorD);
        if (!bottleStorageFull && !bottleInFlight && !sensorA && !sensorB && !sensorC && !sensorD) collectionArmed = true;
        if (!bottleStorageFull && !bottleInFlight && collectionArmed && sensorA && sensorB) {
            if (!validationStarted) validationStarted = now;
            else if (now - validationStarted >= BOTTLE_VALIDATION_TIME_MS && detectedCredits) {
                bottleInFlight = true; bottleCredits = detectedCredits;
                collectionArmed = false;
                validationStarted = 0; door(true);
            }
        } else if (!bottleInFlight) validationStarted = 0;
        if (bottleInFlight) {
            if (detectedCredits > bottleCredits) bottleCredits = detectedCredits;
        }
        if (bottleInFlight && !sensorA && !sensorB && !sensorC && !sensorD) {
            bottleReady = true;
            bottleInFlight = false;
            door(false);
        }
    }
    xSemaphoreGive(hardwareMutex);
}
