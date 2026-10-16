# ESP32 reverse vending machine

Four product slots, two bottle sensors, four selection buttons, four drop sensors, a tube door servo, and a 20×4 I2C LCD. The ESP32 runs its own WiFi access point and serves a small user portal and admin page.

## Before wiring

The old [Plan.jpg](Plan.jpg) is a **concept sketch, not a wiring schematic**. It contains incorrect pin and power labels. Use the table below and verify the actual ESP32 board pinout and sensor signal levels before applying power. ESP32 GPIO is **3.3 V only**. Power servos from a separate, adequate 5 V supply and connect its ground to ESP32 ground. A 5 V PCF8574 LCD backpack needs 3.3 V safe I2C pullups or a bidirectional level shifter.

| Part | GPIO |
| --- | --- |
| Bottle sensors (upper and lower) | 34, 35 |
| Product buttons 1–4 | 13, 14, 27, 26 |
| Drop sensors 1–4 | 16, 17, 32, 33 |
| Dispenser servos 1–4 | 18, 19, 21, 25 |
| Tube door servo | 4 |
| LCD SDA, SCL | 23, 22 |

Buttons use internal pullups and connect to ground when pressed. The drop sensors are configured active LOW. Bottle sensors are configured active HIGH by default; change `BOTTLE_SENSOR_ACTIVE_LEVEL` in [config.h](config.h) only if the installed sensor module is inverted. GPIO 34–36 have **no internal pullups**; open-collector sensors on those pins require external 3.3 V pullups. Set the door angles and continuous-rotation dispenser stop/push duty values in `config.h` for your mechanism before stocking the machine.

## Build and first start

1. Use an ESP32 Arduino core with the pin-based `ledcAttach` / `ledcWrite` API (core 3.x), ArduinoJson 7.x, ESPAsyncWebServer and its compatible AsyncTCP.
2. Open [ArduinoSmartVendingMachine.ino](ArduinoSmartVendingMachine.ino) and upload to a classic ESP32 development board. Upload the contents of [data](data) as the LittleFS filesystem image.
3. Open the 115200 baud Serial Monitor. On first start, record the generated WiFi password and initial `admin@local` password. Known legacy `admin123` admin passwords are rotated on upgrade and the replacement appears in Serial.
4. Connect to `ESP32-Inventory`, open `http://192.168.4.1`, and log in. Opening the admin page synchronizes the machine clock from the admin device (make sure its date and time are correct). Admins can also set **Machine date and time** manually in Philippine local time (UTC+8). Either method lets redemption start without router Wi-Fi. Connecting the machine to a router with internet lets NTP keep time synchronized automatically while the machine hotspot stays available. The admin page also manages products, their fixed servo slots, user credits, and the recent dispense log.

**Back up runtime data before re-uploading a LittleFS image.** Filesystem upload tools can erase the partition containing `/data/users.json`, `/data/products.json`, and the dispense log. Ordinary sketch uploads do not normally replace that filesystem image.

## Dispensing behavior

- The tube door opens after both bottle sensors detect a bottle for 200 ms, then closes once both sensors are clear; it also closes when the session ends or times out.
- A bottle earns one credit when the upper and lower sensors are both active for 200 ms.
- A product button works only while that account's 60-second bottle session is active. The web portal can also request one item.
- The firmware checks stock, credits, and that all drop sensors are clear before starting the selected servo. The first active drop sensor stops all dispensers and confirms delivery. After confirmation, the firmware saves the charge and new stock. A five-second timeout stops the servo without a charge.
- Each account may receive at most four products per Philippine calendar day. Credits expire to zero after two days without account activity. These rules require a valid clock. The admin page sets the clock from the admin device; connecting the machine to internet Wi-Fi keeps it synchronized automatically with NTP.
- Each product keeps its `slot` number when another product is deleted. A new product takes the lowest unused slot.
- The saved Wi-Fi password is not returned by the API or shown in the admin page.

If a stored pin assignment, account file, product file, or pending transaction cannot be safely loaded, startup halts and prints the reason to Serial. It does not format LittleFS or create a known default password over existing data.

## Calibration and verification

Test with empty chutes first. Check each button and slot separately, verify active LOW drop sensors, measure the door travel, then test a successful drop and a jam. Confirm that stock and credits change only on confirmed delivery. Finally test a restart after a completed vend. Keep the machine attended until those tests pass on the actual hardware.

The older [Vending_Machine_Plan.md](Vending_Machine_Plan.md), [connection.md](connection.md), [wiring_and_assembly_guide.md](wiring_and_assembly_guide.md), and [PaanoGumaganaAngVendingMachineFromWebserverUpToDispensing.md](PaanoGumaganaAngVendingMachineFromWebserverUpToDispensing.md) contain historical design notes. This README and current source define the implemented behavior.
