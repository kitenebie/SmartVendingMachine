# 🛠️ Complete Wiring & Assembly Guide — ESP32 Smart Vending Machine (4 Products)

This guide provides an exhaustive, step-by-step mechanical and electrical manual for assembling the **ESP32 38-Pin Smart Vending Machine & Inventory System** illustrated in [`Plan.jpg`](Plan.jpg).

---

## 📋 Table of Contents
1. [Bill of Materials (Hardware Checklist)](#1-bill-of-materials-hardware-checklist)
2. [Master Pinout & Wire Color Table](#2-master-pinout--wire-color-table)
3. [Power Supply Architecture & Common Ground](#3-power-supply-architecture--common-ground)
4. [Sensor Voltage Safety (Level Shifter / Divider)](#4-sensor-voltage-safety-level-shifter--divider)
5. [Step-by-Step Assembly Instructions](#5-step-by-step-assembly-instructions)
   - [Step 1: Power Distribution & Ground Bus](#step-1-power-distribution--ground-bus)
   - [Step 2: Bottle Entry Mechanism (Dual Proximity Sensors)](#step-2-bottle-entry-mechanism-dual-proximity-sensors)
   - [Step 3: Product Compartments & Dispenser Servos](#step-3-product-compartments--dispenser-servos)
   - [Step 4: Product Drop IR Sensors Alignment](#step-4-product-drop-ir-sensors-alignment)
   - [Step 5: Customer Front Panel (Buttons & Display)](#step-5-customer-front-panel-buttons--display)
   - [Step 6: Final ESP32 Connections](#step-6-final-esp32-connections)
6. [Testing & Calibration Procedures](#6-testing--calibration-procedures)
7. [Troubleshooting & Common Pitfalls](#7-troubleshooting--common-pitfalls)

---

## 1. Bill of Materials (Hardware Checklist)

| Item # | Component | Quantity | Specification / Description |
|:---|:---|:---|:---|
| **1** | Main Controller | 1 | **ESP32 38-Pin Development Board** (NodeMCU / DevKit V1) |
| **2** | External Power Supply | 1 | **5V 6A DC Switching Power Supply** (or 12V 5A with step-down buck converter) |
| **3** | Bottle Proximity Sensors | 2 | **NPN Inductive/Capacitive Proximity Sensors** (or IR Obstacle Sensors) |
| **4** | Product Selection Buttons | 4 | **Momentary Push Buttons** (Normal Open / NO) |
| **5** | Dispensing Motors | 4 | **Continuous Rotation Servos (MG996R / DS3218)** with spiral spring coils |
| **6** | Drop Confirmation Sensors | 4 | **IR Beam / Obstacle Sensors** (Active LOW output) |
| **7** | Status Display | 1 | **TM1637 4-Digit 7-Segment Display** (or 20x4 I2C LCD with backpack) |
| **8** | Level Shifter / Resistors | 6 sets | **1kΩ and 2kΩ resistors** (for 5V to 3.3V voltage dividers) |
| **9** | Power Capacitor | 1 | **1000µF 16V Electrolytic Capacitor** (for motor power rail smoothing) |
| **10**| Wiring & Connectors | — | Breadboard / Terminal distribution blocks, Dupont jumper wires |

---

## 2. Master Pinout & Wire Color Table

| Component | Wire Function | Wire Color | ESP32 GPIO | Logic Level | Connection Target |
|:---|:---|:---|:---|:---|:---|
| **Bottle Sensor A (Upper)** | VCC / Power | **RED** | — | 5V | External Power +5V |
| | GND | **BLACK** | — | 0V | Common Ground (GND) |
| | Signal (OUT) | **YELLOW/BLUE** | **GPIO 34** | 3.3V Max | ESP32 Pin 34 (via divider if 5V) |
| **Bottle Sensor B (Lower)** | VCC / Power | **RED** | — | 5V | External Power +5V |
| | GND | **BLACK** | — | 0V | Common Ground (GND) |
| | Signal (OUT) | **YELLOW/BLUE** | **GPIO 35** | 3.3V Max | ESP32 Pin 35 (via divider if 5V) |
| **Button 1 (Product 1)** | Terminal A | **WHITE/BLUE** | **GPIO 13** | 3.3V Pullup | ESP32 Pin 13 |
| | Terminal B | **BLACK** | — | 0V | ESP32 GND |
| **Button 2 (Product 2)** | Terminal A | **WHITE/BLUE** | **GPIO 14** | 3.3V Pullup | ESP32 Pin 14 |
| | Terminal B | **BLACK** | — | 0V | ESP32 GND |
| **Button 3 (Product 3)** | Terminal A | **WHITE/BLUE** | **GPIO 27** | 3.3V Pullup | ESP32 Pin 27 |
| | Terminal B | **BLACK** | — | 0V | ESP32 GND |
| **Button 4 (Product 4)** | Terminal A | **WHITE/BLUE** | **GPIO 26** | 3.3V Pullup | ESP32 Pin 26 |
| | Terminal B | **BLACK** | — | 0V | ESP32 GND |
| **IR Drop Sensor 1** | Signal (OUT) | **YELLOW** | **GPIO 36 (VP)** | 3.3V Active LOW| ESP32 Pin 36 |
| | VCC & GND | **RED / BLACK**| — | 3.3V / 5V | Power Bus & Common GND |
| **IR Drop Sensor 2** | Signal (OUT) | **YELLOW** | **GPIO 39 (VN)** | 3.3V Active LOW| ESP32 Pin 39 |
| | VCC & GND | **RED / BLACK**| — | 3.3V / 5V | Power Bus & Common GND |
| **IR Drop Sensor 3** | Signal (OUT) | **YELLOW** | **GPIO 32** | 3.3V Active LOW| ESP32 Pin 32 |
| | VCC & GND | **RED / BLACK**| — | 3.3V / 5V | Power Bus & Common GND |
| **IR Drop Sensor 4** | Signal (OUT) | **YELLOW** | **GPIO 33** | 3.3V Active LOW| ESP32 Pin 33 |
| | VCC & GND | **RED / BLACK**| — | 3.3V / 5V | Power Bus & Common GND |
| **Servo 1 (Motor 1)** | VCC (Power) | **RED** | — | 5V (High Current)| External 5V Rail |
| | GND | **BROWN/BLACK**| — | 0V | Common GND |
| | Signal (PWM) | **ORANGE/WHITE**| **GPIO 18** | 3.3V PWM Out | ESP32 Pin 18 |
| **Servo 2 (Motor 2)** | VCC (Power) | **RED** | — | 5V (High Current)| External 5V Rail |
| | GND | **BROWN/BLACK**| — | 0V | Common GND |
| | Signal (PWM) | **ORANGE/WHITE**| **GPIO 19** | 3.3V PWM Out | ESP32 Pin 19 |
| **Servo 3 (Motor 3)** | VCC (Power) | **RED** | — | 5V (High Current)| External 5V Rail |
| | GND | **BROWN/BLACK**| — | 0V | Common GND |
| | Signal (PWM) | **ORANGE/WHITE**| **GPIO 21** | 3.3V PWM Out | ESP32 Pin 21 |
| **Servo 4 (Motor 4)** | VCC (Power) | **RED** | — | 5V (High Current)| External 5V Rail |
| | GND | **BROWN/BLACK**| — | 0V | Common GND |
| | Signal (PWM) | **ORANGE/WHITE**| **GPIO 25** | 3.3V PWM Out | ESP32 Pin 25 |
| **7-Segment Display** | VCC & GND | **RED / BLACK**| — | 3.3V / 5V | ESP32 3.3V & GND |
| | CLK (Clock) | **GREEN** | **GPIO 22** | 3.3V Output | ESP32 Pin 22 |
| | DIO (Data) | **YELLOW** | **GPIO 23** | 3.3V I/O | ESP32 Pin 23 |

> 🌐 **Note:** Lahat ng GPIO pins na ito ay **pwedeng i-reconfigure sa Web Dashboard** under `Configuration` page (`/data/configuration.html`).

---

## 3. Power Supply Architecture & Common Ground

> [!CAUTION]
> **CRITICAL RULE FOR MOTOR POWER:**
> Never connect the 4 Servos to the ESP32 onboard 3.3V or VIN pins! When the spring coils turn, current spikes will cause the ESP32 to brown out or reboot.

```text
               +-------------------------------------------------------+
               |               EXTERNAL 5V POWER SUPPLY                |
               |                   (5V, 5A to 6A)                      |
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
               | Signals (Orange/White)|                        |
               |       |       |       |                        |
     +---------+-------+-------+-------+------------------------+-------------+
     |      GPIO 18 GPIO 19 GPIO 21 GPIO 25                                   |
     |                                                   COMMON GND (BLACK)   |
     |                                                          |             |
     |                            ESP32 38-PIN BOARD         [ GND ]          |
     |                                                                        |
     |   5V USB Power Supply ---> [ Micro-USB Port / VIN ]                    |
     +------------------------------------------------------------------------+
```

### Common Ground Principle:
Ikonekta ang **GND wire ng External 5V Power Supply** sa **GND pin ng ESP32**. Kung walang common ground, magiging unstable at manginginig (jitter) ang PWM signal ng mga servo motors.

---

## 4. Sensor Voltage Safety (Level Shifter / Divider)

Ang ESP32 GPIOs ay tumatanggap lamang ng **maximum na 3.3V**. Kung ang iyong proximity sensors ay gumagamit ng 5V signal output, gumawa ng simple voltage divider:

```text
Sensor 5V Signal OUT ────► [ 1.0 kΩ Resistor ] ────┬────► ESP32 GPIO Pin (3.3V Safe)
                                                   │
                                            [ 2.0 kΩ Resistor ]
                                                   │
                                                  GND (Common Ground)
```

---

## 5. Step-by-Step Assembly Instructions

### Step 1: Power Distribution & Ground Bus
1. Ilagay ang External 5V 6A Power Supply sa ilalim o likod ng vending machine enclosure.
2. Gumawa ng **Power Rail (Terminal Block)**:
   - **+5V Rail (Red):** I-distribute papunta sa VCC ng 4x Servos at Sensors. Maglagay ng 1000µF capacitor sa pagitan ng +5V at GND rail.
   - **GND Rail (Black):** I-connect ang Power Supply GND, ESP32 GND, Sensors GND, at Servos GND dito (**Common Ground**).
3. Isaksak ang hiwalay na 5V USB charger sa Micro-USB port ng ESP32 para sa malinis at stable na MCU power.

---

### Step 2: Bottle Entry Mechanism (Dual Proximity Sensors)
1. I-mount ang **Sensor A (Upper)** at **Sensor B (Lower)** sa loob ng bottle insertion chute.
2. Siguraduhing may pagitan ang dalawang sensors na katumbas ng laki ng regular na plastic bottle:
   - Hindi dapat ma-trigger ng maliit na basura ang parehong sensor nang sabay.
3. Ikonekta ang:
   - **Sensor A Signal** ➔ **GPIO 34** (Input)
   - **Sensor B Signal** ➔ **GPIO 35** (Input)
   - VCC at GND sa External Power +5V at Common GND.

---

### Step 3: Product Compartments & Dispenser Servos
1. I-assemble ang 4 na product trays (Compartment 1: Coke, 2: Snacks, 3: Milk, 4: Cup Noodles).
2. Ikabit ang **spiral spring coils** sa shaft ng bawat servo motor.
3. Ikonekta ang PWM signal wires ng servos:
   - **Servo 1 Signal (Orange/White)** ➔ **GPIO 18**
   - **Servo 2 Signal (Orange/White)** ➔ **GPIO 19**
   - **Servo 3 Signal (Orange/White)** ➔ **GPIO 21**
   - **Servo 4 Signal (Orange/White)** ➔ **GPIO 25**
4. Ikonekta ang Red VCC wires sa External 5V Rail at Black/Brown wires sa Common GND.

---

### Step 4: Product Drop IR Sensors Alignment
1. I-mount ang 4 na IR obstacle sensors sa ilalim ng bawat tray chute (bago lumabas sa delivery flap).
2. I-adjust ang potentiometer ng bawat IR sensor module para mag-trigger (active LOW) kapag nahulog ang item.
3. Ikonekta ang:
   - **IR Sensor 1 Signal** ➔ **GPIO 36 (VP)**
   - **IR Sensor 2 Signal** ➔ **GPIO 39 (VN)**
   - **IR Sensor 3 Signal** ➔ **GPIO 32**
   - **IR Sensor 4 Signal** ➔ **GPIO 33**
   - VCC at GND sa 3.3V/5V at Common GND.

---

### Step 5: Customer Front Panel (Buttons & Display)
1. I-drill at i-mount ang **4 na push buttons** sa front panel ng machine:
   - **Button 1 Terminal** ➔ **GPIO 13**, kabilang leg ➔ **GND**
   - **Button 2 Terminal** ➔ **GPIO 14**, kabilang leg ➔ **GND**
   - **Button 3 Terminal** ➔ **GPIO 27**, kabilang leg ➔ **GND**
   - **Button 4 Terminal** ➔ **GPIO 26**, kabilang leg ➔ **GND**
2. I-mount ang **TM1637 7-Segment Display** (o 20x4 I2C LCD):
   - **CLK** ➔ **GPIO 22**
   - **DIO** ➔ **GPIO 23**
   - **VCC** ➔ **3.3V / 5V**
   - **GND** ➔ **GND**

---

### Step 6: Final ESP32 Connections & Flashing
1. I-double check ang lahat ng wiring bago buksan ang kuryente (i-check kung may short circuit).
2. Buksan ang sketch [`ArduinoSmartVendingMachine.ino`](ArduinoSmartVendingMachine.ino) sa **Arduino IDE**.
3. I-flash ang code sa ESP32 board (**Upload** button).
4. I-upload ang `data/` folder sa LittleFS (**Upload LittleFS** tool).

---

## 6. Testing & Calibration Procedures

### Test 1: Serial Monitor Verification
Buksan ang Serial Monitor sa **115200 baud**. Dapat makita ang sumusunod:
```text
========================================
   ESP32 SMART VENDING MACHINE v1.0
   Mode: Hotspot + Web Inventory + Vending
========================================
[Init] LittleFS mounted OK
[Init] Loading GPIO Pin Configuration...
[Init] Administrator account OK
[Init] Products loaded: 4
[Init] Controller initialized with dynamic GPIO pins.
[WiFi] Access Point started! SSID: ESP32-Inventory
>>> VENDING SYSTEM READY & ONLINE! <<<
```

### Test 2: Dual Bottle Sensor Test
1. Harangan ang **Sensor A lang** ➔ Walang credit na dapat pumasok.
2. Harangan ang **Sensor B lang** ➔ Walang credit na dapat pumasok.
3. Ipasok ang bote (harangan ang **Sensor A at B nang sabay**) ➔ Magdaragdag ng `+1 Credit`, tutunog, at magpapakita ng `C 01` sa display.
4. Mananatiling `C 01` habang nakababad ang bote (Anti-double count test passed).

### Test 3: Vending & Drop Confirmation Test
1. Pindutin ang **Button 1** nang may sapat na credit:
   - Magpapakita ang display ng `P 1` ➔ `SALE`.
   - Iikot ang **Servo 1** para itulak ang bote/produkto.
2. Kapag nahulog ang bote at naramdaman ng **IR Sensor 1**:
   - Titigil agad ang Servo 1.
   - Magdi-display ng `DONE`.
   - Mababawasan ang credit (`C 00`) at mababawasan ang stock sa Web Dashboard.

### Test 4: Dispense Timeout (Safety) Test
1. Pindutin ang Button 2 nang walang nakalagay na item sa spiral (o harangan ang coil para hindi mahulog ang item).
2. Pagkatapos ng **5 seconds**:
   - Awtomatikong titigil ang servo.
   - Magdi-display ng `ERR`.
   - **Hindi mababawasan ang credit ng customer at hindi mababawasan ang inventory stock.**

---

## 7. Troubleshooting & Common Pitfalls

| Sintomas / Error | Dahilan | Solusyon |
|:---|:---|:---|
| **Nagre-reboot ang ESP32 kapag umikot ang motor** | Power brownout dahil sa motor current | Huwag kumuha ng 5V power sa ESP32 board; gumamit ng External 5V 6A Power Supply at maglagay ng 1000µF capacitor sa power rail. |
| **Manginginig / hindi gumagalaw ang servo** | Walang common ground sa pagitan ng ESP32 at power supply | Ikonekta ang Power Supply GND sa ESP32 GND pin. |
| **Pumapasok ang credit kahit walang bote** | False trigger sa proximity sensors | I-adjust ang sensitivity potentiometer ng sensor o taasan ang `BOTTLE_VALIDATION_TIME_MS` sa `config.h`. |
| **Hindi tumitigil ang servo kahit nahulog na ang item** | Hindi naka-align ang IR beam ng drop sensor | I-align ang IR transmitter at receiver sa tapat ng chute kung saan tumatama ang nahuhulog na produkto. |
| **Gusto palitan ang pin connection nang hindi nagko-code** | Kailangan ng ibang GPIO mapping | Buksan ang browser sa `http://192.168.4.1` ➔ Pumunta sa **Configuration** page ➔ Palitan ang pin numbers ➔ I-click ang **Save Pin Configuration**. |
