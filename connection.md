# ESP32 Smart Vending Machine & Inventory System — Pin Connection & Setup Manual

This document provides the complete hardware wiring diagram, pin mapping, power isolation rules, and step-by-step procedures to build and connect the **ESP32 38-Pin Smart Vending Machine (4 Products Edition)** with a **20x4 I2C LCD Display (PCF8574 Module)** and integrated **Web Inventory Management**.

> [!TIP]
> **✨ DYNAMIC WEB PIN CONFIGURATION:**
> Lahat ng GPIO pins na nakalista sa ibaba ay **pwedeng baguhin sa Web Interface** (`Configuration` page). Kapag binago at sinave sa web, awtomatiko itong mase-save sa flash memory (`/pins.json`) at ia-apply agad sa hardware nang hindi na kailangang mag-reflash ng code!

---

## 1. Default Pin Mapping Table (4-Product System + 20x4 I2C LCD)

| Component | `+` / VCC connection | `−` / GND connection | Required voltage | Signal connection | Notes |
|:---|:---|:---|:---|:---|:---|
| **Bottle Proximity Sensor A** | External `+5V` | Common GND | `5V` | `OUT` → **GPIO 34 / P34** | `OUT` must be 3.3V max; use a divider/level shifter if its output is 5V. |
| **Bottle Proximity Sensor B** | External `+5V` | Common GND | `5V` | `OUT` → **GPIO 35 / P35** | Same voltage protection as Sensor A; both sensors validate one bottle. |
| **Product 1 Selection Button** | None | One button leg → ESP32 GND | `3.3V` internal pull-up | Other button leg → **GPIO 13 / P13** | Do **not** connect this button to 5V. |
| **Product 2 Selection Button** | None | One button leg → ESP32 GND | `3.3V` internal pull-up | Other button leg → **GPIO 14 / P14** | Do **not** connect this button to 5V. |
| **Product 3 Selection Button** | None | One button leg → ESP32 GND | `3.3V` internal pull-up | Other button leg → **GPIO 27 / P27** | Do **not** connect this button to 5V. |
| **Product 4 Selection Button** | None | One button leg → ESP32 GND | `3.3V` internal pull-up | Other button leg → **GPIO 26 / P26** | Do **not** connect this button to 5V. |
| **IR Drop Sensor 1** | ESP32 `3V3` | Common GND | `3.3V` preferred | `OUT` → **GPIO 16 / P16** | Any active IR sensor stops all servos. |
| **IR Drop Sensor 2** | ESP32 `3V3` | Common GND | `3.3V` preferred | `OUT` → **GPIO 17 / P17** | If the module needs 5V, level-shift/divide its `OUT` to 3.3V. |
| **IR Drop Sensor 3** | ESP32 `3V3` | Common GND | `3.3V` preferred | `OUT` → **GPIO 32 / P32** | Any active IR sensor stops all servos. |
| **IR Drop Sensor 4** | ESP32 `3V3` | Common GND | `3.3V` preferred | `OUT` → **GPIO 33 / P33** | Any active IR sensor stops all servos. |
| **Servo 1 (Spring Motor 1)** | External supply `+5V` | External supply `−` / Common GND | `5V` | Signal (orange/white) → **GPIO 18 / P18** | Never power a servo from ESP32 3V3 or VIN. |
| **Servo 2 (Spring Motor 2)** | External supply `+5V` | External supply `−` / Common GND | `5V` | Signal (orange/white) → **GPIO 19 / P19** | Never power a servo from ESP32 3V3 or VIN. |
| **Servo 3 (Spring Motor 3)** | External supply `+5V` | External supply `−` / Common GND | `5V` | Signal (orange/white) → **GPIO 21 / P21** | Never power a servo from ESP32 3V3 or VIN. |
| **Servo 4 (Spring Motor 4)** | External supply `+5V` | External supply `−` / Common GND | `5V` | Signal (orange/white) → **GPIO 25 / P25** | Never power a servo from ESP32 3V3 or VIN. |
| **20x4 I2C LCD (PCF8574)** | `+5V` from ESP32 VIN or external 5V | Common GND | `5V` | SDA → **GPIO 23 / P23**; SCL → **GPIO 22 / P22** | Put a bidirectional I2C level shifter between LCD SDA/SCL and the ESP32. |

---

> **ESP32 38-pin board labels:** Ang `Pxx` sa board ay katumbas ng `GPIO xx`. Ang IR sensors ay nakalagay sa `P16`, `P17`, `P32`, at `P33`; hindi ginagamit ang `VP` o `VN`. Huwag ikabit ang peripherals sa `SD0`, `SD1`, `SD2`, `SD3`, `CMD`, o `CLK` dahil ginagamit ang mga iyon ng flash memory.

---

## 2. 20x4 I2C LCD Module (PCF8574) Connections

Ang 20x4 Character LCD na may I2C Backpack ay gumagamit lamang ng **4 na wires**:
- **GND** ➔ Ikonekta sa **ESP32 GND / Common GND**
- **VCC** ➔ Ikonekta sa **5V Power (VIN / External 5V)** para sa maliwanag na backlight at contrast
- **SDA** ➔ Ikonekta sa **GPIO 23** (I2C Data)
- **SCL** ➔ Ikonekta sa **GPIO 22** (I2C Clock)

*(I2C Address: Awtomatikong dine-detect ng firmware ang address `0x27` o `0x3F`).*

> [!CAUTION]
> The ESP32 GPIOs are **3.3 V only**. Many PCF8574 LCD backpacks pull SDA and SCL up to their 5 V VCC, which can damage the ESP32. Use a bidirectional I2C level shifter, or move the backpack pull-ups to 3.3 V, while keeping the LCD's VCC at 5 V.

---

## 3. Power Architecture & Wiring Diagram

> [!CAUTION]
> **CRITICAL POWER RULE:**
> **DO NOT power the 4 servo motors from the ESP32 3.3V or 5V VIN pins.**
> 4 servos drawing stall current can pull over 2A–3A. Power them using a dedicated **External 5V 3A–5A Power Supply**.

```text
               +-------------------------------------------------------+
               |               EXTERNAL 5V POWER SUPPLY                |
               |                   (5V, 3A to 6A)                      |
               +---------------------------+---------------------------+
                                           |
               +5V (RED)                   | GND (BLACK)
               |                           |
               +-------+-------+-------+---+--------------------+
               |       |       |       |                        |
               |       |       |       |                  [1000µF Cap]
               |       |       |       |                        |
          +----+---+ +-+--+--+ +-+--+--+ +-+--+--+              |
          | SERVO 1| |SERVO 2| |SERVO 3| |SERVO 4|              |
          +----+---+ +-+--+--+ +-+--+--+ +-+--+--+              |
               |       |       |       |                        |
               | Signals (GPIO)|       |                        |
               |       |       |       |                        |
     +---------+-------+-------+-------+------------------------+-------------+
     |      GPIO 18 GPIO 19 GPIO 21 GPIO 25                                   |
     |                                                   COMMON GND (BLACK)   |
     |                                                          |             |
     |                            ESP32 38-PIN BOARD         [ GND ]          |
     |                                                                        |
     |   5V USB Power ---> [Micro-USB / VIN]                                  |
     |                                                                        |
     |   [GPIO 34] <--- Bottle Sensor A OUT (3.3V safe)                       |
     |   [GPIO 35] <--- Bottle Sensor B OUT (3.3V safe)                       |
     |   [GPIO 16] <--- IR Drop 1 OUT (3.3V safe)                             |
     |   [GPIO 17] <--- IR Drop 2 OUT (3.3V safe)                             |
     |   [GPIO 32] <--- IR Drop 3 OUT (3.3V safe)                             |
     |   [GPIO 33] <--- IR Drop 4 OUT (3.3V safe)                             |
     |   [GPIO 13] <--- Push Button 1 (to GND)                                |
     |   [GPIO 14] <--- Push Button 2 (to GND)                                |
     |   [GPIO 27] <--- Push Button 3 (to GND)                                |
     |   [GPIO 26] <--- Push Button 4 (to GND)                                |
     |   [GPIO 23] <---> 20x4 LCD SDA                                         |
     |   [GPIO 22] ----> 20x4 LCD SCL                                         |
     +------------------------------------------------------------------------+
```

### Common Ground Rule:
Connect the **GND of the External 5V Power Supply** directly to an **ESP32 GND pin**. This creates a shared reference voltage required for PWM servo signals and I2C communication.

---

## 4. Sensor Voltage Safety (Level Shifting)

* ESP32 GPIOs operate at **3.3V logic level**.
* If your Proximity Sensors or IR Sensors output a **5V signal**, use a voltage divider (1kΩ and 2kΩ resistors) or a 5V to 3.3V logic level shifter:
  ```text
  Sensor 5V OUT ---> [ 1kΩ Resistor ] ---> ESP32 GPIO Pin (3.3V)
                                      |
                               [ 2kΩ Resistor ]
                                      |
                                     GND
  ```
* If your sensors output active-low 3.3V signals, you can wire them directly to the GPIO pins.

---

## 5. Hardware Assembly & Connection Procedures

### Step 1: External Power & Ground Sharing
1. Connect your External 5V 3A–6A Power Supply:
   - `+5V` (Red) to the VCC wire of **Servo 1, Servo 2, Servo 3, and Servo 4**.
   - `GND` (Black/Brown) to the GND wire of all 4 servos.
2. Run a wire from the External Power Supply **GND** to any **ESP32 GND** pin.
3. Power the ESP32 via standard USB cable (5V).

### Step 2: Connect the 4 Servos
- **Servo 1 Signal (Orange/White)** → **GPIO 18**
- **Servo 2 Signal (Orange/White)** → **GPIO 19**
- **Servo 3 Signal (Orange/White)** → **GPIO 21**
- **Servo 4 Signal (Orange/White)** → **GPIO 25**

### Step 3: Connect 20x4 I2C LCD Display (PCF8574)
- **VCC** → 5V (ESP32 VIN or External 5V)
- **GND** → Common GND
- **SDA** → **GPIO 23**
- **SCL** → **GPIO 22**
*(I-adjust ang blue potentiometer sa likod ng I2C adapter para sa tamang text contrast).*

> Gumamit ng **bidirectional I2C level shifter** sa SDA at SCL kung 5 V ang VCC ng LCD backpack. Huwag direktang ikabit ang 5 V-pulled-up I2C lines sa ESP32.

### Step 4: Connect the 4 Product Selection Push Buttons
- Connect one leg of **Button 1** to **GPIO 13**, other leg to **GND**.
- Connect one leg of **Button 2** to **GPIO 14**, other leg to **GND**.
- Connect one leg of **Button 3** to **GPIO 27**, other leg to **GND**.
- Connect one leg of **Button 4** to **GPIO 26**, other leg to **GND**.
*(Internal pull-ups are enabled in firmware).*

### Step 5: Connect Bottle Detection Proximity Sensors
- **Sensor A (Upper)** Signal → **GPIO 34**
- **Sensor B (Lower)** Signal → **GPIO 35**
- Sensor VCC & GND to 5V & Common GND.

### Step 6: Connect the 4 IR Drop / Fall Sensors
Mount each IR sensor across the dispensing chute of each product:
- **IR Sensor 1 (Product 1 chute)** Signal → **GPIO 16 (P16)**
- **IR Sensor 2 (Product 2 chute)** Signal → **GPIO 17 (P17)**
- **IR Sensor 3 (Product 3 chute)** Signal → **GPIO 32**
- **IR Sensor 4 (Product 4 chute)** Signal → **GPIO 33**
- IR Sensor VCC & GND to 3.3V/5V & Common GND.

> **Dispensing stop rule:** Habang may dispensing transaction, kahit alin sa apat na IR sensors na maka-detect ng object ay agad magpapahinto sa **lahat ng servos**. Siguraduhing walang object sa harap ng kahit anong IR sensor bago magsimula ang dispensing upang maiwasan ang false successful detection.

---

## 6. 20x4 LCD Screen Layouts

| Screen / Event | Line 1 | Line 2 | Line 3 | Line 4 |
|:---|:---|:---|:---|:---|
| **Idle / Welcome** | `=== SMART VENDING ==` | ` CREDITS: 00 BOTTLE` | `[1]Water    [2]Juice` | `[3]Soda     [4]Snack` |
| **Product Selected** | `SELECT: P1` | `Water        Prc:2` | `Your Credits: 2` | `Checking stock...` |
| **Dispensing (SALE)**| `=== DISPENSING... ==` | `Item: P1 Water     ` | `Please wait...` | `Watch delivery chute` |
| **Successful Drop** | `=== SUCCESSFUL! === ` | `Please take item!` | `Thank you!` | `Credits left: 0` |
| **No Credit (NOCR)** | `== NO ENOUGH CREDIT=` | `Insufficient balance` | `Need:3 \| Have:1` | `Insert more bottles!` |
| **Out of Stock** | `=== OUT OF STOCK! ==` | `P1 Water          ` | `Item is currently` | `unavailable. Sorry!` |
| **Dispense Error** | `== DISPENSE ERROR ==` | `Product 1 jammed   ` | `Credit NOT deducted!` | `Please contact staff` |
