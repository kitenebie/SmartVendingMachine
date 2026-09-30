# 🛠️ Complete Wiring & Assembly Guide — ESP32 Smart Vending Machine (4 Products + 20x4 I2C LCD)

This guide provides an exhaustive, step-by-step mechanical and electrical manual for assembling the **ESP32 38-Pin Smart Vending Machine & Inventory System** with a **20x4 I2C LCD Display (PCF8574 Backpack)** as illustrated in [`Plan.jpg`](Plan.jpg).

---

## 📋 Table of Contents
1. [Bill of Materials (Hardware Checklist)](#1-bill-of-materials-hardware-checklist)
2. [Master Pinout & Wire Color Table](#2-master-pinout--wire-color-table)
3. [20x4 I2C LCD Display (PCF8574) Wiring](#3-20x4-i2c-lcd-display-pcf8574-wiring)
4. [Power Supply Architecture & Common Ground](#4-power-supply-architecture--common-ground)
5. [Sensor Voltage Safety (Level Shifter / Divider)](#5-sensor-voltage-safety-level-shifter--divider)
6. [Step-by-Step Assembly Instructions](#6-step-by-step-assembly-instructions)
7. [20x4 LCD Screen Layout Reference](#7-20x4-lcd-screen-layout-reference)
8. [Testing & Calibration Procedures](#8-testing--calibration-procedures)
9. [Troubleshooting & Common Pitfalls](#9-troubleshooting--common-pitfalls)

---

## 1. Bill of Materials (Hardware Checklist)

| Item # | Component | Quantity | Specification / Description |
|:---|:---|:---|:---|
| **1** | Main Controller | 1 | **ESP32 38-Pin Development Board** (NodeMCU / DevKit V1) |
| **2** | External Power Supply | 1 | **5V 6A DC Switching Power Supply** |
| **3** | Bottle Proximity Sensors | 2 | **NPN Proximity Sensors** (or IR Obstacle Sensors) |
| **4** | Product Selection Buttons | 4 | **Momentary Push Buttons** (Normal Open / NO) |
| **5** | Dispensing Motors | 4 | **Continuous Rotation Servos (MG996R / DS3218)** with spiral spring coils |
| **6** | Drop Confirmation Sensors | 4 | **IR Beam / Obstacle Sensors** (Active LOW output) |
| **7** | Status Display | 1 | **20x4 Character LCD with PCF8574 I2C Adapter** (4 wires: GND, VCC, SDA, SCL) |
| **8** | Level Shifter / Resistors | 6 sets | **1kΩ and 2kΩ resistors** (for 5V to 3.3V voltage dividers) |
| **9** | Power Capacitor | 1 | **1000µF 16V Electrolytic Capacitor** (for motor power rail smoothing) |
| **10**| Wiring & Connectors | — | Terminal blocks, Dupont jumper wires, USB cable |

---

## 2. Master Pinout & Wire Color Table

| Component | Wire Function | Wire Color | ESP32 GPIO | Logic Level | Connection Target |
|:---|:---|:---|:---|:---|:---|
| **Bottle Sensor (Tube Front / Entry)** | VCC / Power | **RED** | — | 5V | External Power +5V |
| | GND | **BLACK** | — | 0V | Common Ground (GND) |
| | Signal (OUT) | **YELLOW/BLUE** | **GPIO 36 / VP** | 3.3V Max | ESP32 Pin VP/GPIO 36 (via divider if 5V) |
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
| **IR Drop Sensor 1** | Signal (OUT) | **YELLOW** | **GPIO 16 (P16)** | 3.3V Active LOW| ESP32 Pin 16 |
| | VCC & GND | **RED / BLACK**| — | 3.3V / 5V | Power Bus & Common GND |
| **IR Drop Sensor 2** | Signal (OUT) | **YELLOW** | **GPIO 17 (P17)** | 3.3V Active LOW| ESP32 Pin 17 |
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
| **Tube Door Servo (180°)** | VCC (Power) | **RED** | — | 5V (High Current) | External 5V Rail |
| | GND | **BROWN/BLACK** | — | 0V | Common GND |
| | Signal (PWM) | **ORANGE/WHITE** | **GPIO 4** | 3.3V PWM Out | ESP32 Pin 4 |
| **20x4 I2C LCD Display** | VCC | **RED** | — | 5V | 5V Power (VIN / Ext 5V) |
| | GND | **BLACK** | — | 0V | Common GND |
| | SDA (Data) | **YELLOW** | **GPIO 23** | 3.3V I2C Data | ESP32 Pin 23 |
| | SCL (Clock) | **GREEN** | **GPIO 22** | 3.3V I2C Clock| ESP32 Pin 22 |

> **ESP32 38-pin silk-screen names:** `Pxx` means GPIO xx (halimbawa, `P23` = GPIO 23), at `VP` ay GPIO 36. Ginagamit ang VP para sa front/entry sensor lamang dahil input-only ito. Ang IR sensors ay gumagamit ng `P16`, `P17`, `P32`, at `P33`. Iwasan ang `SD0`, `SD1`, `SD2`, `SD3`, `CMD`, at `CLK` dahil flash-memory pins ang mga ito.

---

## 3. 20x4 I2C LCD Display (PCF8574) Wiring

Ang 20x4 LCD kasama ang PCF8574 serial adapter module ay nagpapadali ng wiring sa pamamagitan lamang ng **4 na linya**:

```text
    +-------------------------------------------------------+
    |               20x4 I2C LCD BACKPACK                   |
    |          [ GND ]    [ VCC ]    [ SDA ]    [ SCL ]     |
    +-------------+----------+----------+----------+--------+
                  |          |          |          |
                  |          |          |          |
                 GND        +5V       GPIO 23    GPIO 22
                  |       (Power)      (SDA)      (SCL)
                  |          |          |          |
    +-------------+----------+----------+----------+--------+
    |                  ESP32 38-PIN BOARD                   |
    +-------------------------------------------------------+
```

### Contrast Adjustment:
Sa likod ng I2C backpack module, may **asul na potentiometer (trimpot)**. Gamit ang maliit na screwdriver, i-ikot ito hanggang sa maging malinaw at matalas ang mga letra sa asul na screen.

### I2C Logic-Level Safety:
Kahit 5 V ang LCD supply, **hindi 5 V tolerant ang ESP32 GPIOs**. Karamihan ng PCF8574 backpacks ay may pull-up resistors sa VCC; samakatuwid, gumamit ng bidirectional I2C level shifter sa SDA at SCL, o baguhin ang pull-ups upang maging 3.3 V. Huwag direktang ikabit ang 5 V-pulled-up SDA/SCL lines sa GPIO 23 at GPIO 22.

---

## 4. Power Supply Architecture & Common Ground

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

---

## 5. Sensor Voltage Safety (Level Shifter / Divider)

```text
Sensor 5V Signal OUT ────► [ 1.0 kΩ Resistor ] ────┬────► ESP32 GPIO Pin (3.3V Safe)
                                                   │
                                            [ 2.0 kΩ Resistor ]
                                                   │
                                                  GND (Common Ground)
```

---

## 6. Step-by-Step Assembly Instructions

### Step 1: Power Distribution & Ground Bus
1. I-mount ang External 5V 6A Power Supply.
2. Ikonekta ang lahat ng **GND (Power Supply GND + ESP32 GND + Sensors GND + Servos GND + LCD GND)** sa iisang Common Ground terminal.
3. Ikonekta ang 1000µF capacitor sa pagitan ng +5V at GND rail.

### Step 2: Bottle Entry Chute (Entry Sensor, Dual Validation, and Door)
1. I-mount ang front/entry sensor sa pinakaunahan ng tube, at ang Sensor A (Upper) at Sensor B (Lower) sa validation area.
2. Ikonekta ang front/entry sensor ➔ **GPIO 36 (VP)**, Sensor A ➔ **GPIO 34**, at Sensor B ➔ **GPIO 35**.
3. Ikonekta ang signal ng 180° tube-door servo ➔ **GPIO 4**. Ang VCC nito ay dapat sa hiwalay at sapat na 5 V supply; pag-isahin ang GND ng supply at ESP32.

### Step 3: Product Compartments & Dispenser Servos
1. Ikabit ang 4 na spiral spring coils sa 4 na servos.
2. Ikonekta ang PWM signals:
   - Servo 1 ➔ **GPIO 18**, Servo 2 ➔ **GPIO 19**, Servo 3 ➔ **GPIO 21**, Servo 4 ➔ **GPIO 25**.
3. Ikonekta ang servo power wires sa External 5V rail at Common GND.

### Step 4: IR Drop Detection Sensors
1. I-mount ang 4 na IR drop sensors sa ilalim ng dispensing chute.
2. Ikonekta ang:
   - IR 1 ➔ **GPIO 16 (P16)**, IR 2 ➔ **GPIO 17 (P17)**, IR 3 ➔ **GPIO 32 (P32)**, IR 4 ➔ **GPIO 33 (P33)**.

> **Stop behavior:** Habang nagdi-dispense, kapag kahit alin sa IR 1–IR 4 ay naka-detect ng object, agad ihihinto ng firmware ang **lahat ng apat na servos**. Dapat malinaw ang daanan at walang naka-detect na object sa lahat ng IR sensors bago mag-dispense.

### Step 5: Front Panel (4 Push Buttons & 20x4 LCD)
1. I-mount ang 4 na push buttons:
   - Button 1 ➔ **GPIO 13**, Button 2 ➔ **GPIO 14**, Button 3 ➔ **GPIO 27**, Button 4 ➔ **GPIO 26** (kabilang terminals sa GND).
2. I-mount ang 20x4 LCD:
   - **GND** ➔ Common GND
   - **VCC** ➔ 5V Power
   - **SDA** ➔ I2C level shifter ➔ **GPIO 23**
   - **SCL** ➔ I2C level shifter ➔ **GPIO 22**

---

## 7. 20x4 LCD Screen Layout Reference

```text
+--------------------+   +--------------------+
|=== SMART VENDING ==|   |SELECT: P1          |
| CREDITS: 02 BOTTLE |   |Water        Prc:2  |
|[1]Water    [2]Juice|   |Your Credits: 2     |
|[3]Soda     [4]Snack|   |Checking stock...   |
+--------------------+   +--------------------+
     (IDLE / MENU)          (PRODUCT SELECTED)

+--------------------+   +--------------------+
|=== DISPENSING... ==|   |=== SUCCESSFUL! === |
|Item: P1 Water      |   |Please take item!   |
|Please wait...      |   |Thank you!          |
|Watch delivery chute|   |Credits left: 0     |
+--------------------+   +--------------------+
     (DISPENSING)               (SUCCESS)

+--------------------+   +--------------------+
|== NO ENOUGH CREDIT=|   |=== OUT OF STOCK! ==|
|Insufficient balance|   |P1 Water            |
|Need:3 | Have:1     |   |Item is currently   |
|Insert more bottles!|   |unavailable. Sorry! |
+--------------------+   +--------------------+
 (INSUFFICIENT CREDIT)       (OUT OF STOCK)
```

---

## 8. Testing & Calibration Procedures

1. **Serial Monitor Check:** Buksan ang 115200 baud Serial Monitor. Siguraduhing lumabas ang `[LCD] Found I2C LCD at address 0x27` (o `0x3F`).
2. **Bottle Entry Test:** Habang naka-detect ang front/entry sensor, walang dagdag na credit at nakasara ang door. Kapag clear na ito at sabay na naka-detect ang Sensor A at B nang 200 ms, bubukas ang door at magbabago ang Line 2 ng LCD sa `CREDITS: 01 BOTTLE`.
3. **Dispense & Drop Test:** Pindutin ang Button 1. Magdi-display ang LCD ng `DISPENSING...` at iikot ang Servo 1. Kapag may object na na-detect ang kahit alin sa IR Sensor 1–4, titigil ang lahat ng servos at magdi-display ang LCD ng `SUCCESSFUL!`.
4. **Safety Timeout Test:** Subukang mag-dispense kapag walang item. Pagkaraan ng 5 segundo, titigil ang servo at magdi-display ng `DISPENSE ERROR` nang walang bawas sa credits o stock.
