# ESP32 Smart Vending Machine & Inventory System — Pin Connection & Setup Manual

This document provides the complete hardware wiring diagram, pin mapping, power isolation rules, and step-by-step procedures to build and connect the **ESP32 38-Pin Smart Vending Machine (4 Products Edition)** with integrated **Web Inventory Management**.

> [!TIP]
> **✨ DYNAMIC WEB PIN CONFIGURATION:**
> Lahat ng GPIO pins na nakalista sa ibaba ay **pwedeng baguhin sa Web Interface** (`Configuration` page). Kapag binago at sinave sa web, awtomatiko itong mase-save sa flash memory (`/pins.json`) at ia-apply agad sa hardware nang hindi na kailangang mag-reflash ng code!

---

## 1. Default Pin Mapping Table (4-Product System)

| Component | Component Pin / Wire | ESP32 GPIO / Pin | Pin Type | Notes |
|:---|:---|:---|:---|:---|
| **Bottle Proximity Sensor A** | Signal (OUT) | **GPIO 34** | Input | Validates bottle insertion with Sensor B |
| **Bottle Proximity Sensor B** | Signal (OUT) | **GPIO 35** | Input | Must detect bottle simultaneously |
| **Product 1 Selection Button** | Signal (NO) | **GPIO 13** | Input (Pull-up) | Connect other pin of button to GND |
| **Product 2 Selection Button** | Signal (NO) | **GPIO 14** | Input (Pull-up) | Connect other pin of button to GND |
| **Product 3 Selection Button** | Signal (NO) | **GPIO 27** | Input (Pull-up) | Connect other pin of button to GND |
| **Product 4 Selection Button** | Signal (NO) | **GPIO 26** | Input (Pull-up) | Connect other pin of button to GND |
| **Product 1 IR Drop Sensor** | Signal (OUT) | **GPIO 36 (VP)** | Input | Confirms Product 1 fell down |
| **Product 2 IR Drop Sensor** | Signal (OUT) | **GPIO 39 (VN)** | Input | Confirms Product 2 fell down |
| **Product 3 IR Drop Sensor** | Signal (OUT) | **GPIO 32** | Input | Confirms Product 3 fell down |
| **Product 4 IR Drop Sensor** | Signal (OUT) | **GPIO 33** | Input | Confirms Product 4 fell down |
| **Servo 1 (Spring Motor 1)** | Signal (Orange/White) | **GPIO 18** | PWM Output | Dispenses Product 1 |
| **Servo 2 (Spring Motor 2)** | Signal (Orange/White) | **GPIO 19** | PWM Output | Dispenses Product 2 |
| **Servo 3 (Spring Motor 3)** | Signal (Orange/White) | **GPIO 21** | PWM Output | Dispenses Product 3 |
| **Servo 4 (Spring Motor 4)** | Signal (Orange/White) | **GPIO 25** | PWM Output | Dispenses Product 4 |
| **7-Segment Display (TM1637)**| CLK | **GPIO 22** | Output | Display Clock |
| **7-Segment Display (TM1637)**| DIO | **GPIO 23** | I/O | Display Data |

---

## 2. Power Architecture & Wiring Diagram

> [!CAUTION]
> **CRITICAL POWER RULE:**
> **DO NOT power the 4 servo motors from the ESP32 3.3V or 5V VIN pins.**
> 4 servos drawing stall current can pull over 2A–3A. Power them using a dedicated **External 5V 3A–5A Power Supply**.

```text
               +-------------------------------------------------------+
               |               EXTERNAL 5V POWER SUPPLY                |
               |                   (5V, 3A to 5A)                      |
               +---------------------------+---------------------------+
                                           |
               +5V (RED)                   | GND (BLACK)
               |                           |
               +-------+-------+-------+---+--------------------+
               |       |       |       |                        |
               |       |       |       |                        |
          +----+---+ +-+--+--+ +-+--+--+ +-+--+--+              |
          | SERVO 1| |SERVO 2| |SERVO 3| |SERVO 4|              |
          +----+---+ +-+--+--+ +-+--+--+ +-+--+--+              |
               |       |       |       |                        |
               | Signals (GPIO)|       |                        |
               |       |       |       |                        |
     +---------+-------+-------+-------+------------------------+-------------+
     |         |       |       |       |                        |             |
     |      GPIO 18 GPIO 19 GPIO 21 GPIO 25                     |             |
     |                                                          |             |
     |                                                   COMMON GND (BLACK)   |
     |                                                          |             |
     |                            ESP32 38-PIN BOARD            |             |
     |                                                          |             |
     |   5V USB Power ---> [Micro-USB / VIN]                 [ GND ]          |
     |                                                                        |
     |   [GPIO 34] <--- Bottle Sensor A OUT (3.3V safe)                       |
     |   [GPIO 35] <--- Bottle Sensor B OUT (3.3V safe)                       |
     |   [GPIO 36] <--- IR Drop 1 OUT (3.3V safe)                             |
     |   [GPIO 39] <--- IR Drop 2 OUT (3.3V safe)                             |
     |   [GPIO 32] <--- IR Drop 3 OUT (3.3V safe)                             |
     |   [GPIO 33] <--- IR Drop 4 OUT (3.3V safe)                             |
     |   [GPIO 13] <--- Push Button 1 (to GND)                                |
     |   [GPIO 14] <--- Push Button 2 (to GND)                                |
     |   [GPIO 27] <--- Push Button 3 (to GND)                                |
     |   [GPIO 26] <--- Push Button 4 (to GND)                                |
     |   [GPIO 22] ---> 7-Segment CLK                                         |
     |   [GPIO 23] ---> 7-Segment DIO                                         |
     +------------------------------------------------------------------------+
```

### Common Ground Rule:
Connect the **GND of the External 5V Power Supply** directly to an **ESP32 GND pin**. This creates a shared reference voltage required for PWM servo signals.

---

## 3. Sensor Voltage Safety (Level Shifting)

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

## 4. Hardware Assembly & Connection Procedures

### Step 1: External Power & Ground Sharing
1. Connect your External 5V 3A+ Power Supply:
   - `+5V` (Red) to the VCC wire of **Servo 1, Servo 2, Servo 3, and Servo 4**.
   - `GND` (Black/Brown) to the GND wire of all 4 servos.
2. Run a wire from the External Power Supply **GND** to any **ESP32 GND** pin.
3. Power the ESP32 via standard USB cable (5V).

### Step 2: Connect the 4 Servos
- **Servo 1 Signal (Orange/White)** → **GPIO 18**
- **Servo 2 Signal (Orange/White)** → **GPIO 19**
- **Servo 3 Signal (Orange/White)** → **GPIO 21**
- **Servo 4 Signal (Orange/White)** → **GPIO 25**

### Step 3: Connect 7-Segment Display (TM1637)
- **VCC** → ESP32 3.3V or VIN (5V)
- **GND** → ESP32 GND
- **CLK** → **GPIO 22**
- **DIO** → **GPIO 23**

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
- **IR Sensor 1 (Product 1 chute)** Signal → **GPIO 36 (VP)**
- **IR Sensor 2 (Product 2 chute)** Signal → **GPIO 39 (VN)**
- **IR Sensor 3 (Product 3 chute)** Signal → **GPIO 32**
- **IR Sensor 4 (Product 4 chute)** Signal → **GPIO 33**
- IR Sensor VCC & GND to 3.3V/5V & Common GND.
