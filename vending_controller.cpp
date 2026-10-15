#include "vending_controller.h"
#include "config.h"
#include "pin_config.h"
#include "display_manager.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

static SemaphoreHandle_t hardwareMutex;
static bool collectionEnabled = false, bottleInFlight = false, bottleReady = false, doorOpen = false;
static uint32_t validationStarted = 0, dispenseStartedAt = 0;
static uint8_t dispensingSlot = 0, buttonPress = 0;
static uint8_t buttonRaw[4], buttonStable[4];
static uint32_t buttonChangedAt[4];
static DispenseResult dispenseResult = DispenseResult::None;
static const uint32_t SERVO_MIN = 3277, SERVO_MAX = 6553;

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
    pinMode(p.sensorA, INPUT); pinMode(p.sensorB, INPUT);
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
    if (!enabled) {
        validationStarted = 0; bottleInFlight = false; bottleReady = false;
        door(false);
    }
    xSemaphoreGive(hardwareMutex);
}
bool consumeValidatedBottle() {
    xSemaphoreTake(hardwareMutex, portMAX_DELAY);
    bool result = bottleReady;
    bottleReady = false;
    xSemaphoreGive(hardwareMutex);
    return result;
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
        bool sensorA = digitalRead(p.sensorA) == BOTTLE_SENSOR_ACTIVE_LEVEL;
        bool sensorB = digitalRead(p.sensorB) == BOTTLE_SENSOR_ACTIVE_LEVEL;
        if (!bottleInFlight && sensorA && sensorB) {
            if (!validationStarted) validationStarted = now;
            else if (now - validationStarted >= BOTTLE_VALIDATION_TIME_MS) {
                bottleInFlight = true; bottleReady = true; validationStarted = 0; door(true);
            }
        } else if (!bottleInFlight) validationStarted = 0;
        if (!sensorA && !sensorB) {
            bottleInFlight = false;
            door(false);
        }
    }
    xSemaphoreGive(hardwareMutex);
}
