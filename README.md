# ESP32 Smart Vending Machine & Inventory Management System

A complete web-based inventory management system and automatic 4-product vending machine hosted directly on an **ESP32 38-Pin Development Board**.

The ESP32 operates in **WiFi Access Point (Hotspot) mode** and acts as both the Web Server and the Hardware Controller — no external router or internet required!

---

## 📐 Complete System Architecture & Wiring Diagram

![Vending Machine System - Complete Diagram](Plan.jpg)

---

## 🌟 Key Features

- **Stand-alone WiFi Hotspot & Web Server:** Connect directly to `ESP32-Inventory` WiFi and open `http://192.168.4.1`.
- **4-Product Dispensing System:**
  - **4x Selection Push Buttons** (with 50ms software debouncing).
  - **4x High-Torque Servos / Spring Dispensing Coils** (with 5-second stall protection timeout).
  - **4x Product Drop IR Sensors** (confirms physical drop before deducting credit & stock).
- **Dual Proximity Sensor Bottle Credit System:**
  - Sensor A (Upper) + Sensor B (Lower) must detect the bottle simultaneously to award `+1 Credit`.
  - Anti-double counting lock prevents multiple credits from a single bottle.
- **Hardware Status Display (7-Segment TM1637 / I2C LCD):**
  - Live status codes: `C 00` (Credits), `P1`–`P4` (Product Selected), `SALE` (Dispensing), `DONE` (Success), `NOCR` (No Credit), `EMPT` (Out of Stock), `ERR` (Timeout Error).
- **Web-Based GPIO Pin Management:**
  - Fully reconfigurable GPIO pins directly from the web browser (`Configuration` page).
  - Saved persistently to `/pins.json` in LittleFS and applied live to hardware without re-flashing!
- **Persistent Storage:**
  - Product database and stock saved in **LittleFS** (`/products.json`).
  - Administrator credentials stored in **ESP32 NVS/Preferences** partition.
- **Modern Responsive Web UI:** Real-time dashboard, product CRUD, dynamic pricing (in credits/₱), stock tracking, and account security.

---

## 📂 Project Structure

```text
ESP32_Smart_Vending_Machine/
├── ArduinoSmartVendingMachine.ino  <- Main sketch (or ESP32_Inventory_System.ino)
├── config.h                        <- Hotspot settings & default GPIO pin definitions
├── pin_config.h / .cpp             <- Dynamic GPIO pin storage & live manager (/pins.json)
├── vending_controller.h / .cpp     <- State machine, dual bottle sensor logic, buttons & servos
├── display_manager.h / .cpp        <- 7-Segment (TM1637) & hardware status display
├── wifi_manager.h / .cpp           <- WiFi Access Point (Hotspot) handler
├── storage.h / .cpp                <- LittleFS filesystem helper
├── auth.h / .cpp                   <- NVS admin authentication & session tokens
├── products.h / .cpp               <- Product CRUD database & LittleFS persistence
├── api.h / .cpp                    <- REST API endpoints & route handlers
├── connection.md                   <- Complete hardware wiring & manual setup guide
├── Plan.jpg                        <- Complete visual circuit & mechanical diagram
└── data/                           <- Web frontend assets (HTML, CSS, JS)
    ├── index.html
    ├── login.html
    ├── dashboard.html
    ├── products.html
    ├── configuration.html
    ├── css/
    │   └── style.css
    └── js/
        ├── auth.js
        ├── dashboard.js
        ├── products.js
        └── configuration.js
```

---

## 🔌 Default GPIO Pin Mapping (4 Products)

| Component | Default GPIO | Type | Description |
|:---|:---|:---|:---|
| **Bottle Sensor A (Upper)** | `GPIO 34` | Input | Proximity sensor A |
| **Bottle Sensor B (Lower)** | `GPIO 35` | Input | Proximity sensor B |
| **Button 1 (Product 1)** | `GPIO 13` | Input (Pull-up) | Push button for Product 1 |
| **Button 2 (Product 2)** | `GPIO 14` | Input (Pull-up) | Push button for Product 2 |
| **Button 3 (Product 3)** | `GPIO 27` | Input (Pull-up) | Push button for Product 3 |
| **Button 4 (Product 4)** | `GPIO 26` | Input (Pull-up) | Push button for Product 4 |
| **IR Drop Sensor 1** | `GPIO 36` (VP) | Input | Drop confirmation Product 1 |
| **IR Drop Sensor 2** | `GPIO 39` (VN) | Input | Drop confirmation Product 2 |
| **IR Drop Sensor 3** | `GPIO 32` | Input | Drop confirmation Product 3 |
| **IR Drop Sensor 4** | `GPIO 33` | Input | Drop confirmation Product 4 |
| **Servo 1 (Motor 1)** | `GPIO 18` | PWM Out | Spring coil dispenser 1 |
| **Servo 2 (Motor 2)** | `GPIO 19` | PWM Out | Spring coil dispenser 2 |
| **Servo 3 (Motor 3)** | `GPIO 21` | PWM Out | Spring coil dispenser 3 |
| **Servo 4 (Motor 4)** | `GPIO 25` | PWM Out | Spring coil dispenser 4 |
| **7-Segment Display (CLK)** | `GPIO 22` | Output | TM1637 Clock |
| **7-Segment Display (DIO)** | `GPIO 23` | I/O | TM1637 Data |

> 💡 *Note: All pins above can be modified via the Web Dashboard without re-flashing!*

---

## ⚡ Power Supply & Common Ground

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
     +------------------------------------------------------------------------+
```

* **Servos:** Powered via external 5V 3A–6A power supply.
* **Common Ground:** Connect the External Power Supply **GND** to the **ESP32 GND**.

---

## 🚀 Setup & Upload Instructions (Arduino IDE)

1. Open **Arduino IDE**.
2. Open [`ArduinoSmartVendingMachine.ino`](ArduinoSmartVendingMachine.ino) (or [`ESP32_Inventory_System.ino`](ESP32_Inventory_System.ino)).
3. Install required libraries from Library Manager (`Ctrl + Shift + I`):
   - **`ArduinoJson`** by *Benoît Blanchon* (v7.x)
   - **`AsyncTCP`** by *ESP32Async*
   - **`ESPAsyncWebServer`** by *ESP32Async*
4. Select board **DOIT ESP32 DEVKIT V1** and your COM Port.
5. Click **Upload (➡)** to flash the sketch.
6. Upload the `data/` folder to LittleFS using the Arduino IDE LittleFS upload tool (*Tools → ESP32 LittleFS Data Upload* or *Upload LittleFS*).

---

## 📱 How to Use & Connect

1. Power on the ESP32 and external motor power supply.
2. Connect your phone, tablet, or PC to the WiFi network:
   - **WiFi Name (SSID):** `ESP32-Inventory`
   - **WiFi Password:** `inventory123` *(or open if configured with `""`)*
3. Open any web browser and go to:
   ```text
   http://192.168.4.1
   ```
4. Log in with the default administrator credentials:
   - **Username:** `admin`
   - **Password:** `admin123`

---

## 📖 Additional Documentation

- **[`connection.md`](connection.md)** — Complete step-by-step wiring guide, level shifting, and physical assembly procedures.
- **[`Vending_Machine_Plan.md`](Vending_Machine_Plan.md)** — Technical design specifications and transaction state machine documentation.
