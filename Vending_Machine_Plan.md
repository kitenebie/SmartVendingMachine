# ESP32 Vending Machine Plan

> [!IMPORTANT]
> **Current firmware implementation (4 products):** The active code uses four buttons, four servos, four IR sensors, and a 20x4 I2C LCD with a PCF8574 backpack. On the ESP32 38-pin board, `Pxx` means GPIO xx; `VP` is GPIO 36 and `VN` is GPIO 39. The `SD0`, `SD1`, `SD2`, `SD3`, `CMD`, and `CLK` pins are reserved for flash memory. While dispensing, detection from **any one** of the four IR sensors stops **all servos**; the transaction then applies to the selected product. Older conceptual three-product examples below are historical only where they differ from this notice.

## 1. Project Overview

Build an ESP32 38-pin based vending machine with these core functions:

- Detect an inserted bottle using two proximity sensors.
- Both proximity sensors must detect the same bottle before adding +1 credit.
- Display credits and vending status on a 20x4 I2C LCD with a PCF8574 adapter.
- Provide 3 product-selection buttons.
- Each button corresponds to one product and one dispensing servo.
- Each servo drives a spring/pusher mechanism.
- Each product has an IR drop sensor.
- The IR sensor confirms that the product actually fell.
- Stop the servo when the product is detected.
- Deduct credits and stock only after successful delivery.
- Use a timeout to prevent a servo from running indefinitely.

---

## 2. System Architecture

```text
                         ┌──────────────────────┐
                         │      ESP32 38-Pin    │
                         │                      │
                         │  Credit Controller   │
                         │  Product Controller  │
                         │  Servo Controller    │
                         │  Sensor Controller   │
                         │  Display Controller  │
                         └──────────┬───────────┘
                                    │
          ┌─────────────────────────┼────────────────────────┐
          │                         │                        │
          ▼                         ▼                        ▼
  2x Bottle Sensors          3x Product Buttons       20x4 I2C LCD
  Sensor A + Sensor B        Button 1 → Product 1     Credits
                             Button 2 → Product 2     Product
                             Button 3 → Product 3     Status
          │                         │
          │                         ▼
          │                 3x Servo + Spring
          │                         │
          │                         ▼
          │                  Product falls
          │                         │
          │                         ▼
          │                  3x IR Sensors
          │                  Delivery Confirm
          │
          ▼
     Credit +1
```

---

## 3. Hardware Components

### Main Controller

- ESP32 38-pin development board

### Bottle Detection

- 2x proximity sensors
- Sensor A + Sensor B must both detect the bottle

### Product Selection

- 3x push buttons

```text
Button 1 → Product 1
Button 2 → Product 2
Button 3 → Product 3
```

### Product Dispensing

- 3x servo motors
- 3x spring/pusher mechanisms

```text
Servo 1 → Spring 1 → Product 1
Servo 2 → Spring 2 → Product 2
Servo 3 → Spring 3 → Product 3
```

### Product Delivery Confirmation

- 3x IR sensors

```text
IR 1 → Product 1 delivery
IR 2 → Product 2 delivery
IR 3 → Product 3 delivery
```

### User Feedback

Use a 20x4 character LCD with a PCF8574 I2C adapter (GND, VCC, SDA, and SCL).

The display should show:

- Current credits
- Selected product
- Dispensing status
- Success
- Error
- Insufficient credit
- Out of stock

---

## 4. Bottle Credit Detection

### Requirement

The two proximity sensors must detect the bottle together, within a short validation window.

Do not add credit when only one sensor detects the bottle.

### Invalid

```text
Sensor A = ON
Sensor B = OFF

Result:
No credit
```

### Valid

```text
Sensor A = ON
Sensor B = ON

Result:
Credit +1
```

Confirm the condition for approximately 100–300 ms to reduce false triggering.

---

## 5. Prevent Double Counting

One bottle must only add one credit.

When both sensors detect a bottle:

```text
Sensor A = ON
Sensor B = ON
       ↓
Credit +1
       ↓
Detection LOCK
```

Remain locked while the bottle is still detected.

Wait until:

```text
Sensor A = OFF
Sensor B = OFF
```

Then unlock.

Example:

```text
00 → 11 → +1 → 11 → 11 → 00
                   │
                   └── No additional credit
```

Next bottle:

```text
00 → 11 → +1
```

---

## 6. Credit Management

Credits represent the number of validated bottles inserted.

Example:

```text
Bottle 1 → +1
Bottle 2 → +1
Bottle 3 → +1
```

Result:

```text
Credits = 3
```

For a normal vending session, credits should be held in RAM and reset after reboot unless persistent credits are specifically required.

---

## 7. Product Configuration

Each product should have:

```text
Product ID
Product Name
Price
Stock
Servo
IR Sensor
Button
```

Example:

```text
Product 1
Name: Water
Price: 2 credits
Stock: 10
Servo: Servo 1
IR: IR 1
Button: Button 1
```

```text
Product 2
Name: Juice
Price: 3 credits
Stock: 10
Servo: Servo 2
IR: IR 2
Button: Button 2
```

```text
Product 3
Name: Soda
Price: 4 credits
Stock: 10
Servo: Servo 3
IR: IR 3
Button: Button 3
```

Actual names, prices, and stock values should be configurable.

---

## 8. Product Selection

```text
Button 1 → Product 1
Button 2 → Product 2
Button 3 → Product 3
```

When a button is pressed:

```text
Button pressed
      ↓
Select Product
      ↓
Display Product
      ↓
Check Stock
      ↓
Check Credits
```

Ignore product-selection buttons while another product is being dispensed.

---

## 9. Product Validation

Before starting the servo:

1. Verify the product exists.
2. Verify stock is greater than zero.
3. Verify credits are greater than or equal to the product price.

Check:

```text
Credits >= Product Price
```

Only if all checks pass:

```text
START DISPENSING
```

If stock is zero:

```text
EMPT
```

If credits are insufficient:

```text
NOCR
```

---

## 10. Servo Dispensing Mechanism

Each product has a dedicated servo.

```text
Product 1 → Servo 1 → Spring/Pusher 1
Product 2 → Servo 2 → Spring/Pusher 2
Product 3 → Servo 3 → Spring/Pusher 3
```

When dispensing begins:

```text
Servo → PUSH POSITION
```

The spring pushes the selected product toward the dispensing chute.

---

## 11. IR Delivery Confirmation

Any one of the four IR sensors can confirm that an object was detected during dispensing.

The system must not consider the transaction successful simply because the servo moved.

Correct sequence:

```text
Start Servo
    ↓
Spring pushes product
    ↓
Product falls
    ↓
Any IR Sensor detects object
    ↓
SUCCESS
```

---

## 12. Stop Servo After Successful Detection

When any one of the four IR sensors detects an object during an active dispense:

```text
IR Sensor = DETECTED
       ↓
Stop All Servos
       ↓
Transaction SUCCESS
```

Example:

```text
Button 1
   ↓
Servo 1 START
   ↓
Product 1 falls
   ↓
Any IR 1–IR 4 detects
   ↓
All Servos STOP
```

The selected product remains the product whose credit and stock are updated. Ensure all IR sensors are clear before starting a transaction to avoid a false confirmation.

---

## 13. Credit Deduction Rule

Credits must be deducted only after successful delivery.

### Incorrect

```text
Button pressed
    ↓
Deduct credit
    ↓
Start servo
```

### Correct

```text
Button pressed
    ↓
Check credits
    ↓
Check stock
    ↓
Start servo
    ↓
Wait for IR confirmation
    ↓
Product detected
    ↓
SUCCESS
    ↓
Deduct credits
    ↓
Decrease stock
```

This prevents the user from losing credits when a product gets stuck.

---

## 14. Stock Deduction Rule

Stock must also be deducted only after successful delivery.

Example:

```text
Initial Stock = 10

Dispensing starts
        ↓
Product falls
        ↓
IR detected
        ↓
Stock = 9
```

If dispensing fails:

```text
Stock remains 10
```

---

## 15. Dispensing Timeout

The servo must never run indefinitely.

Recommended initial timeout:

```text
DISPENSE_TIMEOUT = 5 seconds
```

Flow:

```text
Servo START
    ↓
Wait for IR
    ↓
IR detected?
```

### YES

```text
Stop Servo
    ↓
SUCCESS
```

### NO

After timeout:

```text
Stop Servo
    ↓
Transaction FAILED
```

---

## 16. Failed Dispense

If the product is not detected before timeout:

```text
Servo STOP
Credits NOT deducted
Stock NOT deducted
```

Display:

```text
ERR
```

Then return to the waiting state.

---

## 17. Transaction State Machine

Use a state machine instead of putting all logic into one large `loop()`.

Recommended states:

```text
IDLE
CREDIT_VALIDATION
WAITING_FOR_SELECTION
CHECKING_PRODUCT
DISPENSING
SUCCESS
FAILED
INSUFFICIENT_CREDIT
OUT_OF_STOCK
```

Flow:

```text
                 ┌──────────────┐
                 │     IDLE     │
                 └──────┬───────┘
                        │
                 Bottle detected
                        │
                        ▼
              ┌──────────────────┐
              │ CREDIT_VALIDATION│
              └────────┬─────────┘
                       │
                   Credit +1
                       │
                       ▼
             ┌────────────────────┐
             │ WAITING_SELECTION  │
             └─────────┬──────────┘
                       │
                 Button pressed
                       │
                       ▼
             ┌────────────────────┐
             │ CHECKING_PRODUCT   │
             └─────────┬──────────┘
                       │
             ┌─────────┼─────────┐
             │         │         │
          No Stock  No Credit   Valid
             │         │         │
             ▼         ▼         ▼
          OUT_STOCK  NO_CREDIT  DISPENSING
                                  │
                         ┌────────┴────────┐
                         │                 │
                     IR detected       Timeout
                         │                 │
                         ▼                 ▼
                      SUCCESS            FAILED
                         │                 │
                         ▼                 ▼
                      Deduct            No Deduct
                      Credit            No Stock Change
                         │
                         ▼
                        IDLE
```

---

## 18. 20x4 I2C LCD Display

Recommended display states:

| State | LCD message |
|---|---|
| Idle | Credits and product menu |
| Product selected | Product name, price, and available credits |
| Dispensing | Product and delivery progress |
| Success | Confirmation and remaining credits |
| No credit | Required and available credits |
| Out of stock | Product unavailable notice |
| Timeout/error | Dispensing error notice |

Example:

```text
=== SMART VENDING ==
 CREDITS: 03 BOTTLE
```

Button 2 pressed:

```text
SELECT: P2
```

Dispensing:

```text
=== DISPENSING... ==
```

Successful delivery:

```text
DONE
```

Then:

```text
C 01
```

---

## 19. Complete Transaction Example

Initial:

```text
Credits = 0
Product 1 Stock = 10
Product 1 Price = 2
```

Bottle inserted:

```text
Sensor A = ON
Sensor B = ON
```

System:

```text
Credits = 1
```

Another bottle:

```text
Credits = 2
```

Display:

```text
C 02
```

User presses Button 1:

```text
P1
```

System checks:

```text
Credits = 2
Price = 2
Stock = 10
```

All valid.

Servo starts:

```text
Servo 1 = PUSH
```

Product falls.

Any IR sensor detects:

```text
IR 1, IR 2, IR 3, or IR 4 = ON
```

System:

```text
All servos = STOP
```

Transaction:

```text
SUCCESS
```

Then:

```text
Credits = 0
Stock = 9
```

Display:

```text
C 00
```

---

## 20. Insufficient Credit Example

Product 1:

```text
Price = 3
```

Current credits:

```text
Credits = 2
```

Button 1:

```text
Button 1 pressed
       ↓
Credits = 2
Price = 3
       ↓
2 < 3
```

Result:

```text
Servo does not start
```

Display:

```text
NOCR
```

Credits remain:

```text
2
```

---

## 21. Out of Stock Example

Product 2:

```text
Stock = 0
```

Button 2:

```text
Button 2 pressed
       ↓
Stock = 0
```

Result:

```text
Servo 2 does not start
```

Display:

```text
EMPT
```

Credits remain unchanged.

---

## 22. Dispensing Failure Example

Button 3 is pressed with valid credit and stock.

```text
Servo 3 starts
       ↓
Product becomes stuck
       ↓
No IR sensor detects an object
       ↓
5-second timeout
```

System:

```text
Servo 3 = STOP
Credits unchanged
Stock unchanged
```

Display:

```text
ERR
```

---

## 23. Button Debouncing

All three physical buttons must use debounce logic.

Recommended debounce time:

```text
30–50 ms
```

Sequence:

```text
Button press
    ↓
Debounce
    ↓
Confirm still pressed
    ↓
Process once
```

This prevents one physical press from triggering multiple transactions.

---

## 24. Sensor Debouncing

### Bottle Sensors

```text
Both sensors ON
    ↓
Wait 100–300 ms
    ↓
Both still ON?
    ↓
YES → Valid bottle
```

### Product IR Sensor

```text
IR ON
    ↓
Confirm signal
    ↓
Stop servo
```

---

## 25. One Transaction at a Time

For the first version, only one dispensing operation should run at a time.

While:

```text
DISPENSING
```

other product buttons should be ignored.

Example:

```text
Servo 1 dispensing
      ↓
Button 2 pressed
      ↓
Ignore Button 2
```

After success or timeout:

```text
Return to WAITING_SELECTION
```

This prevents conflicting servo operations.

---

## 26. Power Architecture

Do not power the three servos directly from ESP32 GPIO pins.

Use a separate suitable 5V power supply for the servos.

```text
                5V POWER SUPPLY
                       │
          ┌────────────┼────────────┐
          │            │            │
       Servo 1      Servo 2      Servo 3
          │            │            │
          └────────────┼────────────┘
                       │
                      GND
                       │
                 ESP32 GND
```

The ESP32 and external servo supply must share a common ground.

The servo power supply should be sized for the combined current of all three servos, including startup and stall current.

---

## 27. Sensor Voltage Safety

Before connecting sensors to ESP32 GPIO:

1. Identify the exact sensor model.
2. Determine its operating voltage.
3. Determine its output voltage.
4. Verify that the output is 3.3V-safe.

ESP32 GPIO is not designed for arbitrary 5V logic input.

If a sensor outputs 5V logic, use an appropriate level-shifting circuit or voltage divider.

Do not connect an unknown 5V output directly to an ESP32 GPIO.

---

## 28. Suggested GPIO Allocation

The exact GPIO numbers should be finalized after confirming the actual ESP32 board and peripherals.

Logical allocation:

```text
INPUTS

Bottle Sensor A  → GPIO
Bottle Sensor B  → GPIO

Button 1         → GPIO
Button 2         → GPIO
Button 3         → GPIO

Drop IR 1        → GPIO
Drop IR 2        → GPIO
Drop IR 3        → GPIO


OUTPUTS

Servo 1          → GPIO
Servo 2          → GPIO
Servo 3          → GPIO

20x4 I2C LCD      → SDA and SCL GPIOs
```

Before coding, create a final GPIO table and verify no conflicts with:

- Boot/strapping pins
- Flash
- Serial communication
- Display interface
- Servo control
- Sensor inputs

---

## 29. Software Architecture

Recommended firmware structure:

```text
ESP32-Vending/
│
├── src/
│   ├── main.cpp
│   ├── config.h
│   │
│   ├── credit_manager.cpp
│   ├── credit_manager.h
│   │
│   ├── product_manager.cpp
│   ├── product_manager.h
│   │
│   ├── button_manager.cpp
│   ├── button_manager.h
│   │
│   ├── servo_manager.cpp
│   ├── servo_manager.h
│   │
│   ├── sensor_manager.cpp
│   ├── sensor_manager.h
│   │
│   ├── display_manager.cpp
│   ├── display_manager.h
│   │
│   ├── vending_controller.cpp
│   └── vending_controller.h
│
└── README.md
```

---

## 30. Module Responsibilities

### Credit Manager

Responsible for:

- Bottle detection
- Credit addition
- Duplicate-credit prevention
- Current credit count
- Credit deduction

Example functions:

```cpp
detectBottle();
addCredit();
getCredits();
deductCredits();
resetCredits();
```

### Product Manager

Responsible for:

- Product name
- Price
- Stock
- Product ID
- Stock decrement

Example:

```cpp
getProduct();
checkStock();
decreaseStock();
```

### Button Manager

Responsible for:

- Button 1
- Button 2
- Button 3
- Debouncing

### Servo Manager

Responsible for:

- Servo 1
- Servo 2
- Servo 3
- Start push
- Stop servo
- Timeout

### Sensor Manager

Responsible for:

- Bottle Sensor A
- Bottle Sensor B
- Drop IR 1
- Drop IR 2
- Drop IR 3

### Display Manager

Responsible for:

- Credits
- Product selection
- Success
- Error
- Out of stock
- Insufficient credit

### Vending Controller

Responsible for the complete transaction state machine.

---

## 31. Core Pseudocode

```cpp
loop() {

    updateBottleSensors();
    updateButtons();
    updateDropSensors();

    switch (vendingState) {

        case IDLE:
            if (validBottleDetected()) {
                addCredit();
                displayCredits();
                vendingState = WAITING_SELECTION;
            }
            break;

        case WAITING_SELECTION:
            if (button1Pressed()) {
                selectProduct(1);
            }

            if (button2Pressed()) {
                selectProduct(2);
            }

            if (button3Pressed()) {
                selectProduct(3);
            }

            break;

        case CHECKING_PRODUCT:

            if (!hasStock(selectedProduct)) {
                displayOutOfStock();
                vendingState = WAITING_SELECTION;
                break;
            }

            if (!hasEnoughCredits(selectedProduct)) {
                displayInsufficientCredit();
                vendingState = WAITING_SELECTION;
                break;
            }

            startDispensing(selectedProduct);
            vendingState = DISPENSING;

            break;

        case DISPENSING:

            if (anyDropSensorDetected()) {

                stopAllServos();

                deductCredits(selectedProduct);
                decreaseStock(selectedProduct);

                displaySuccess();

                vendingState = SUCCESS;
            }

            else if (dispenseTimeout()) {

                stopAllServos();

                displayError();

                vendingState = FAILED;
            }

            break;

        case SUCCESS:

            displayCredits();
            vendingState = WAITING_SELECTION;
            break;

        case FAILED:

            displayCredits();
            vendingState = WAITING_SELECTION;
            break;
    }
}
```

---

## 32. Core Transaction Rules

The following rules must always be enforced:

1. Both bottle sensors must detect the bottle before credit is added.
2. One bottle can only generate one credit.
3. The system must wait for both bottle sensors to clear before another bottle is counted.
4. Product buttons must be debounced.
5. Product selection must check stock.
6. Product selection must check credits.
7. Servo must not start if credits are insufficient.
8. Servo must not start if stock is zero.
9. Any one of the four IR sensors confirms an object during active dispensing.
10. All servos stop immediately after successful IR detection.
11. Servo must stop after a maximum timeout.
12. Credits are deducted only after successful delivery.
13. Stock is deducted only after successful delivery.
14. Credits and stock are not deducted when dispensing fails.
15. Only one product should be dispensed at a time.
16. Servo power must come from a suitable external supply.
17. ESP32 and external servo supply must share GND.
18. Sensor outputs must be confirmed safe for ESP32 GPIO.

---

## 33. Complete System Flow

```text
POWER ON
   ↓
Initialize ESP32
   ↓
Initialize sensors
   ↓
Initialize buttons
   ↓
Initialize servos
   ↓
Initialize 20x4 I2C LCD
   ↓
Load product configuration
   ↓
Display current credits
   ↓
        IDLE
         │
         ▼
Bottle enters
         │
         ▼
Sensor A + Sensor B detected
         │
         ▼
Validate detection
         │
         ▼
Credit +1
         │
         ▼
Display credits
         │
         ▼
Wait for button
         │
    ┌────┼────┐
    ▼    ▼    ▼
   P1   P2   P3
    │    │    │
    └────┼────┘
         ▼
Check stock
         │
         ▼
Check credits
         │
         ├───────────────┐
         │               │
       FAIL             PASS
         │               │
         ▼               ▼
     Show Error      Start Servo
                         │
                         ▼
                    Push Product
                         │
                         ▼
                   Product Falls
                         │
                         ▼
                  IR Sensor Detects
                         │
                  ┌──────┴──────┐
                  │             │
                 YES            NO
                  │             │
                  ▼             ▼
               SUCCESS       TIMEOUT
                  │             │
                  ▼             ▼
          Stop All Servos  Stop All Servos
                  │             │
                  ▼             ▼
             Credit - Price   No Deduction
                  │           No Stock Change
                  ▼
              Stock - 1
                  │
                  ▼
              Update LCD
                  │
                  ▼
                 IDLE
```

---

## 34. Future Web Management System

The vending machine can later be integrated with the planned web application.

Possible web modules:

```text
Dashboard
Products
Configuration
Inventory
Sales
Credits
Vending Logs
```

Product configuration:

```text
Product ID
Name
Price
Stock
Servo
IR Sensor
Button
```

This allows product prices and stock to be changed without modifying firmware.

---

## 35. Development Phases

### Phase 1 — Hardware Test

- ESP32
- Proximity sensors
- Buttons
- Servos
- IR sensors
- 20x4 I2C LCD

Test every component independently.

### Phase 2 — Bottle Detection

Implement:

- Sensor A
- Sensor B
- Dual-sensor validation
- Credit +1
- Anti-double-counting

### Phase 3 — Product Selection

Implement:

- Button 1
- Button 2
- Button 3
- Product selection
- Display selection

### Phase 4 — Servo Control

Implement:

- Servo 1
- Servo 2
- Servo 3
- Spring/pusher mechanism

### Phase 5 — Product Detection

Implement:

- IR 1
- IR 2
- IR 3
- Successful delivery detection
- Servo stop

### Phase 6 — Transaction Logic

Implement:

- Credit validation
- Stock validation
- Dispensing
- Success
- Timeout
- Failed transaction

### Phase 7 — Display

Implement:

- Credits
- Product
- SALE
- DONE
- NOCR
- EMPT
- ERR

### Phase 8 — Web Management

Implement:

- Login
- Dashboard
- Product management
- Configuration
- Price management
- Stock management
- Transaction logs

---

## 36. Final Expected Behavior

```text
1. User inserts a bottle.
2. Proximity Sensor A detects it.
3. Proximity Sensor B detects it.
4. ESP32 validates both sensors.
5. Credit increases by 1.
6. LCD updates the credit display.
7. User presses Product 1, 2, or 3.
8. ESP32 checks product stock.
9. ESP32 checks available credits.
10. If invalid, display an error and do not move the servo.
11. If valid, start the selected servo.
12. Servo pushes the spring.
13. Product falls.
14. Any one of the four IR sensors detects an object.
15. All servos immediately stop.
16. Transaction is marked successful.
17. Credits are deducted.
18. Product stock is reduced by 1.
19. LCD displays remaining credits.
20. Machine returns to the waiting state.
21. If none of the IR sensors detects an object within the timeout, stop all servos and do not deduct credits or stock.
```

---

## 37. Success Criteria

- [ ] ESP32 boots correctly.
- [ ] Both bottle sensors can be read.
- [ ] A valid bottle adds exactly +1 credit.
- [ ] One bottle cannot generate multiple credits.
- [ ] The system waits for both sensors to clear before another bottle is counted.
- [ ] Button 1 selects Product 1.
- [ ] Button 2 selects Product 2.
- [ ] Button 3 selects Product 3.
- [ ] Insufficient credits prevent dispensing.
- [ ] Empty stock prevents dispensing.
- [ ] Correct servo activates for the selected product.
- [ ] Spring pushes the product.
- [ ] Any IR sensor detection stops all servos during dispensing.
- [ ] All servos stop after successful detection.
- [ ] Successful delivery deducts credits.
- [ ] Successful delivery reduces stock.
- [ ] Failed delivery does not deduct credits.
- [ ] Failed delivery does not reduce stock.
- [ ] Servo timeout works.
- [ ] LCD shows credits.
- [ ] LCD shows selected product.
- [ ] LCD shows success/error states.
- [ ] Only one transaction runs at a time.
- [ ] Servo power is properly isolated from ESP32 GPIO power.
- [ ] All sensor signal voltages are safe for ESP32 GPIO.
