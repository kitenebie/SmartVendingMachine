// ============================================================
//  vending_controller.cpp -- Vending Controller State Machine
//  Integrates 4-Product Dispensing with 20x4 I2C LCD Display
// ============================================================
#include "vending_controller.h"
#include "config.h"
#include "pin_config.h"
#include "display_manager.h"
#include "products.h"

// ---- State variables ---------------------------------------
static VendingState s_state = STATE_IDLE;
static int s_credits = 0;
static int s_selectedProductIndex = -1; // 1, 2, 3, or 4
static uint32_t s_stateTimer = 0;
static uint32_t s_bottleDetectionStart = 0;
static uint32_t s_doorOpenedAt = 0;
static bool s_bottleLock = false;
static bool s_doorOpen = false;

// Button debounce tracking (4 buttons)
static uint32_t s_lastBtn1Press = 0;
static uint32_t s_lastBtn2Press = 0;
static uint32_t s_lastBtn3Press = 0;
static uint32_t s_lastBtn4Press = 0;

// Servo PWM configuration (ESP32 LEDC PWM)
static const int SERVO_PWM_FREQ = 50;     // 50Hz for standard servos
static const int SERVO_PWM_RES  = 16;     // 16-bit resolution (0-65535)

static const uint32_t SERVO_DUTY_STOP = 4915; // ~1.5ms neutral pulse
static const uint32_t SERVO_DUTY_PUSH = 6553; // ~2.0ms push pulse
static const uint32_t SERVO_DUTY_MIN  = 3277; // ~1.0ms pulse (0 degrees)
static const uint32_t SERVO_DUTY_MAX  = 6553; // ~2.0ms pulse (180 degrees)

static uint32_t servoDutyForAngle(int angle) {
    angle = constrain(angle, 0, 180);
    return SERVO_DUTY_MIN + ((SERVO_DUTY_MAX - SERVO_DUTY_MIN) * angle) / 180;
}

static void setDoorOpen(bool open) {
    if (s_doorOpen == open) return;

    const PinConfig& p = getPinConfig();
    ledcWrite(p.doorServo, servoDutyForAngle(open ? DOOR_OPEN_ANGLE : DOOR_CLOSED_ANGLE));
    s_doorOpen = open;
    if (open) s_doorOpenedAt = millis();
    Serial.printf("[Vending] Bottle tube door %s.\n", open ? "opened" : "closed");
}

static void setServoDuty(int productIndex, uint32_t duty) {
    const PinConfig& p = getPinConfig();
    if (productIndex == 1)      ledcWrite(p.servo1, duty);
    else if (productIndex == 2) ledcWrite(p.servo2, duty);
    else if (productIndex == 3) ledcWrite(p.servo3, duty);
    else if (productIndex == 4) ledcWrite(p.servo4, duty);
}

static void stopAllServos() {
    const PinConfig& p = getPinConfig();
    ledcWrite(p.servo1, SERVO_DUTY_STOP);
    ledcWrite(p.servo2, SERVO_DUTY_STOP);
    ledcWrite(p.servo3, SERVO_DUTY_STOP);
    ledcWrite(p.servo4, SERVO_DUTY_STOP);
}

// ------------------------------------------------------------
void initVendingMachine() {
    const PinConfig& p = getPinConfig();

    // Proximity Sensors
    pinMode(p.sensorEntry, INPUT);
    pinMode(p.sensorA, INPUT);
    pinMode(p.sensorB, INPUT);

    // Push Buttons (Internal Pull-Ups, active LOW)
    pinMode(p.btn1, INPUT_PULLUP);
    pinMode(p.btn2, INPUT_PULLUP);
    pinMode(p.btn3, INPUT_PULLUP);
    pinMode(p.btn4, INPUT_PULLUP);

    // Drop IR Sensors (Active LOW when obstacle/item detected)
    pinMode(p.ir1, INPUT);
    pinMode(p.ir2, INPUT);
    pinMode(p.ir3, INPUT);
    pinMode(p.ir4, INPUT);

    // Setup LEDC PWM on Servo Pins
    ledcAttach(p.servo1, SERVO_PWM_FREQ, SERVO_PWM_RES);
    ledcAttach(p.servo2, SERVO_PWM_FREQ, SERVO_PWM_RES);
    ledcAttach(p.servo3, SERVO_PWM_FREQ, SERVO_PWM_RES);
    ledcAttach(p.servo4, SERVO_PWM_FREQ, SERVO_PWM_RES);
    ledcAttach(p.doorServo, SERVO_PWM_FREQ, SERVO_PWM_RES);
    stopAllServos();
    s_doorOpen = true; // Force the initial command below even after a live pin reconfiguration.
    setDoorOpen(false);

    // Initialize 20x4 I2C LCD Display
    initDisplay();
    displayCredits(s_credits);

    Serial.println("[Vending] Controller initialized with 20x4 I2C LCD.");
}

int getCurrentCredits() {
    return s_credits;
}

void addCredit(int amount) {
    s_credits += amount;
    Serial.printf("[Vending] Credit added! Total credits: %d\n", s_credits);
    displayCredits(s_credits);
}

void resetCredits() {
    s_credits = 0;
    displayCredits(s_credits);
}

String getVendingStateName() {
    switch (s_state) {
        case STATE_IDLE:                 return "IDLE";
        case STATE_CREDIT_VALIDATION:    return "CREDIT_VALIDATION";
        case STATE_WAITING_SELECTION:    return "WAITING_SELECTION";
        case STATE_CHECKING_PRODUCT:     return "CHECKING_PRODUCT";
        case STATE_DISPENSING:           return "DISPENSING";
        case STATE_SUCCESS:              return "SUCCESS";
        case STATE_FAILED:               return "FAILED";
        case STATE_INSUFFICIENT_CREDIT:  return "INSUFFICIENT_CREDIT";
        case STATE_OUT_OF_STOCK:         return "OUT_OF_STOCK";
        default:                         return "UNKNOWN";
    }
}

// Return the first IR sensor detecting an object (Active LOW on standard IR modules).
// Any of the four sensors is allowed to stop an active dispensing operation.
static int getTriggeredIrSensor() {
    const PinConfig& p = getPinConfig();
    if (digitalRead(p.ir1) == LOW) return 1;
    if (digitalRead(p.ir2) == LOW) return 2;
    if (digitalRead(p.ir3) == LOW) return 3;
    if (digitalRead(p.ir4) == LOW) return 4;
    return 0;
}

// ------------------------------------------------------------
// Main non-blocking update routine
// ------------------------------------------------------------
void updateVendingMachine() {
    uint32_t now = millis();
    const PinConfig& p = getPinConfig();

    // ---- 1. BOTTLE DETECTION LOGIC -------------------------
    bool entryDetected = (digitalRead(p.sensorEntry) == HIGH);
    bool sA = (digitalRead(p.sensorA) == HIGH);
    bool sB = (digitalRead(p.sensorB) == HIGH);

    // A bottle at the tube entrance must never award a credit.  It has to
    // leave the entry sensor, then be confirmed by BOTH validation sensors.
    if (!s_bottleLock && !entryDetected && sA && sB) {
        if (s_bottleDetectionStart == 0) {
            s_bottleDetectionStart = now;
        } else if (now - s_bottleDetectionStart >= BOTTLE_VALIDATION_TIME_MS) {
            // Valid bottle: release the door first, then award exactly one credit.
            setDoorOpen(true);
            addCredit(1);
            s_bottleLock = true;
            s_bottleDetectionStart = 0;
            if (s_state == STATE_IDLE) {
                s_state = STATE_WAITING_SELECTION;
            }
        }
    } else if (!s_bottleLock) {
        // Entry occupied, or only one validation sensor active: no credit.
        s_bottleDetectionStart = 0;
    }

    // Keep the door open while the accepted bottle clears the tube.  The
    // minimum open period avoids closing on the bottle before it reaches the door.
    if (s_bottleLock && !entryDetected && !sA && !sB &&
        now - s_doorOpenedAt >= DOOR_MIN_OPEN_TIME_MS) {
        setDoorOpen(false);
        s_bottleLock = false;
        s_bottleDetectionStart = 0;
    }

    // ---- 2. BUTTON INPUT PROCESSING (4 BUTTONS) ------------
    if (s_state == STATE_IDLE || s_state == STATE_WAITING_SELECTION) {
        if (digitalRead(p.btn1) == LOW && (now - s_lastBtn1Press > BTN_DEBOUNCE_MS)) {
            s_lastBtn1Press = now;
            s_selectedProductIndex = 1;
            s_state = STATE_CHECKING_PRODUCT;
        } else if (digitalRead(p.btn2) == LOW && (now - s_lastBtn2Press > BTN_DEBOUNCE_MS)) {
            s_lastBtn2Press = now;
            s_selectedProductIndex = 2;
            s_state = STATE_CHECKING_PRODUCT;
        } else if (digitalRead(p.btn3) == LOW && (now - s_lastBtn3Press > BTN_DEBOUNCE_MS)) {
            s_lastBtn3Press = now;
            s_selectedProductIndex = 3;
            s_state = STATE_CHECKING_PRODUCT;
        } else if (digitalRead(p.btn4) == LOW && (now - s_lastBtn4Press > BTN_DEBOUNCE_MS)) {
            s_lastBtn4Press = now;
            s_selectedProductIndex = 4;
            s_state = STATE_CHECKING_PRODUCT;
        }
    }

    // ---- 3. STATE MACHINE ----------------------------------
    switch (s_state) {
        case STATE_IDLE:
        case STATE_WAITING_SELECTION:
            break;

        case STATE_CHECKING_PRODUCT: {
            const auto& prods = getProducts();
            Product* itemPtr = nullptr;
            for (auto& item : const_cast<std::vector<Product>&>(prods)) {
                if (item.id == (uint32_t)s_selectedProductIndex) {
                    itemPtr = &item;
                    break;
                }
            }

            int requiredCredits = itemPtr ? (int)itemPtr->price : 1;
            int stockAvailable  = itemPtr ? itemPtr->stocks : 0;
            String prodName     = itemPtr ? itemPtr->name : ("Product " + String(s_selectedProductIndex));

            displayProductSelected(s_selectedProductIndex, prodName, (float)requiredCredits, s_credits);

            if (!itemPtr || stockAvailable <= 0) {
                Serial.printf("[Vending] Product %d is OUT OF STOCK!\n", s_selectedProductIndex);
                displayEmpty(s_selectedProductIndex, prodName);
                s_stateTimer = now;
                s_state = STATE_OUT_OF_STOCK;
                break;
            }

            if (s_credits < requiredCredits) {
                Serial.printf("[Vending] Insufficient credits! Need %d, have %d\n", requiredCredits, s_credits);
                displayNoCredit(requiredCredits, s_credits);
                s_stateTimer = now;
                s_state = STATE_INSUFFICIENT_CREDIT;
                break;
            }

            // Start Dispensing
            Serial.printf("[Vending] Dispensing Product %d (%s)...\n", s_selectedProductIndex, prodName.c_str());
            displaySale(s_selectedProductIndex, prodName);
            setServoDuty(s_selectedProductIndex, SERVO_DUTY_PUSH);
            s_stateTimer = now;
            s_state = STATE_DISPENSING;
            break;
        }

        case STATE_DISPENSING: {
            int triggeredIrSensor = getTriggeredIrSensor();
            if (triggeredIrSensor != 0) {
                stopAllServos();
                Serial.printf("[Vending] IR sensor %d detected an object; all servos stopped. Product %d delivered!\n",
                              triggeredIrSensor, s_selectedProductIndex);

                Product* itemPtr = getProductById((uint32_t)s_selectedProductIndex);
                if (itemPtr) {
                    s_credits -= (int)itemPtr->price;
                    if (s_credits < 0) s_credits = 0;
                    if (itemPtr->stocks > 0) itemPtr->stocks--;
                    saveProducts();
                } else {
                    s_credits--;
                }

                displayDone(s_credits);
                s_stateTimer = now;
                s_state = STATE_SUCCESS;
            } else if (now - s_stateTimer >= DISPENSE_TIMEOUT_MS) {
                stopAllServos();
                Serial.printf("[Vending] ERROR: Dispense timeout for Product %d!\n", s_selectedProductIndex);
                displayError(s_selectedProductIndex);
                s_stateTimer = now;
                s_state = STATE_FAILED;
            }
            break;
        }

        case STATE_SUCCESS:
        case STATE_FAILED:
        case STATE_INSUFFICIENT_CREDIT:
        case STATE_OUT_OF_STOCK:
            // Display status message for 2.5 seconds, then return to waiting/idle
            if (now - s_stateTimer >= 2500) {
                displayCredits(s_credits);
                s_state = (s_credits > 0) ? STATE_WAITING_SELECTION : STATE_IDLE;
                s_selectedProductIndex = -1;
            }
            break;

        default:
            s_state = STATE_IDLE;
            break;
    }
}

bool triggerDispense(int productIndex) {
    if (s_state == STATE_DISPENSING) return false;
    if (productIndex < 1 || productIndex > 4) return false;
    s_selectedProductIndex = productIndex;
    s_state = STATE_CHECKING_PRODUCT;
    return true;
}
