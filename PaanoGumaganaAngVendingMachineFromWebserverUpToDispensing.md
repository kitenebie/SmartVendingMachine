# Paano Gumagana ang Vending Machine: Mula Web Server Hanggang Dispensing

Ang ESP32 ang utak ng vending machine. Mayroon itong dalawang magkaugnay na tungkulin:

- Magpatakbo ng sariling WiFi hotspot at web-based inventory dashboard.
- Magbasa ng sensors at buttons, magpatakbo ng servo, at mag-update ng 20x4 I2C LCD.

Hindi kailangan ng internet o external router. Ang phone o computer ay kumokonekta direkta sa WiFi hotspot ng ESP32.

## 1. Ano ang nangyayari sa pag-on ng ESP32

Nasa `setup()` ng `ArduinoSmartVendingMachine.ino` ang startup sequence.

1. Sinisimulan ang Serial Monitor sa `115200` baud.
2. Mina-mount ang LittleFS, ang flash storage ng ESP32 para sa web files at data.
3. Kinukuha ang GPIO configuration mula sa `/pins.json`; kung wala pa ito, default pins ang ginagamit.
4. Kino-configure ang administrator account sa NVS/Preferences.
5. Kinukuha ang products mula sa `/products.json`. Kapag wala pa ang file, awtomatikong gumagawa ng apat na default products.
6. Sinisimulan ang vending hardware: buttons, bottle sensors, IR sensors, servos, at 20x4 I2C LCD.
7. Sinisimulan ang WiFi hotspot, pagkatapos ay ang web-server routes.

Kapag handa na, paulit-ulit na tinatawag ng `loop()` ang `updateVendingMachine()`. Ito ang nagpapatakbo ng non-blocking vending state machine.

## 2. Hotspot at web server

Ang `wifi_manager.cpp` ay gumagamit ng `WiFi.softAP()` para gumawa ng hotspot gamit ang settings sa `config.h`:

| Setting | Default value |
|---|---|
| WiFi name | `ESP32-Inventory` |
| Password | `inventory123` |
| IP address | `192.168.4.1` |
| Maximum clients | 4 |

Pagkatapos kumonekta ang phone o computer sa hotspot, buksan ang `http://192.168.4.1`. Ang `ESPAsyncWebServer` ay nagse-serve ng mga HTML, CSS, at JavaScript file mula sa LittleFS `data/` folder.

```text
Phone / PC
    │  WiFi
    ▼
ESP32 hotspot (ESP32-Inventory)
    │  HTTP: http://192.168.4.1
    ▼
ESPAsyncWebServer
    ├── Web pages mula LittleFS
    └── REST API para sa products, pins, account, at dispensing
```

## 3. Login at session protection

Ang login page ay nagpapadala ng username at password sa `POST /api/login`.

1. Ang `api.cpp` ay tumatawag sa `validateCredentials()`.
2. Ang credentials ay naka-store sa ESP32 NVS sa `auth.cpp`.
3. Kapag tama ang login, gumagawa ang ESP32 ng random session token.
4. Itinatago ng browser ang token at ipinapadala ito bilang `X-Session-Token` sa mga susunod na API request.
5. Bine-verify ng `isAuthenticated()` ang token bago payagan ang inventory, pin configuration, o remote dispense request.

Ang session ay nasa RAM at nag-e-expire pagkatapos ng walong oras na walang activity. Kapag nag-restart ang ESP32, kailangang mag-login muli.

## 4. Product at inventory management sa web dashboard

Ang dashboard at Products page ay kumukuha ng data gamit ang `GET /api/products`.

Ang bawat product ay may sumusunod na impormasyon:

| Field | Kahulugan |
|---|---|
| `id` | Product number/identifier |
| `name` | Pangalan na makikita sa web at LCD |
| `stocks` | Bilang ng natitirang item |
| `price` | Credits na kailangan para mabili ang item |

Maaaring magdagdag, mag-edit, o mag-delete ng products sa web page. Ang mga request ay dumadaan sa REST API:

| Gawain | API endpoint |
|---|---|
| Tingnan ang products at machine status | `GET /api/products` |
| Magdagdag ng product | `POST /api/products` |
| I-edit ang product | `PUT /api/products/{id}` |
| Mag-delete ng product | `DELETE /api/products/{id}` |

Pagkatapos ng pagbabago, ang `products.cpp` ay nagsa-save ng listahan sa `/products.json`. Ibig sabihin, hindi mawawala ang stock at presyo kapag ni-restart ang ESP32.

Ang `GET /api/products` response ay kasama rin ang `machineCredits` at `machineState`, kaya nakikita ng web application ang kasalukuyang credits at estado ng machine.

## 5. Paano nakakakuha ng credits ang customer

Ang credits ay hindi awtomatikong galing sa web dashboard. Sa normal na customer flow, galing ito sa dalawang bottle sensors:

1. Nababasa ng `updateVendingMachine()` ang Sensor A at Sensor B.
2. Kailangan parehong active ang dalawang sensor nang hindi bababa sa `BOTTLE_VALIDATION_TIME_MS` (default: 200 ms).
3. Kapag valid ang detection, tatawag ang firmware sa `addCredit(1)`.
4. Madaragdagan ng isang credit ang `s_credits` at maa-update ang LCD.
5. Naka-lock muna ang bottle detector hanggang mawala ang bote sa parehong sensor. Ito ang anti-double-count protection.

Sa idle screen ng 20x4 LCD, makikita ang credits at pagpipilian ng products.

```text
=== SMART VENDING ==
 CREDITS: 02 BOTTLE
[1]Water    [2]Juice
[3]Soda     [4]Snack
```

## 6. Pagpili ng product: physical button at web request

May dalawang paraan upang mag-request ng dispensing:

- **Physical button:** Kapag pinindot ang Button 1–4, pipiliin ng firmware ang katumbas na product ID.
- **REST API:** Ang authenticated `POST /api/vending/dispense` na may body na `{"productId": 1}` ay tumatawag sa `triggerDispense(1)`.

Pareho silang nagtatakda ng state sa `STATE_CHECKING_PRODUCT`. Hindi agad umiikot ang servo sa pag-click o pagpindot; dadaan muna ang request sa stock at credit validation.

> Ang kasalukuyang dashboard ay pang-monitor at inventory management. Available ang remote dispense endpoint sa firmware para sa ibang web UI o integration na gagamit nito.

## 7. Validation bago paandarin ang servo

Sa `STATE_CHECKING_PRODUCT`, hinahanap ng firmware ang product gamit ang product ID at sinusuri ang sumusunod:

1. **May product ba na tumutugma sa ID?** Kung wala, ituturing itong out of stock.
2. **May stock ba?** Kapag `stocks <= 0`, hindi aandar ang servo at magpapakita ang LCD ng out-of-stock message.
3. **Sapat ba ang credits?** Kapag mas mababa ang `s_credits` sa `price`, hindi aandar ang servo at magpapakita ang LCD ng required at available credits.

Kapag pumasa ang lahat, ipinapakita ng LCD ang product at dispensing status, saka lamang magpapatuloy sa `STATE_DISPENSING`.

```text
Product request
      │
      ▼
May product at may stock? ── Hindi ──► OUT OF STOCK
      │ Oo
      ▼
Sapat ang credits? ───────── Hindi ──► INSUFFICIENT CREDIT
      │ Oo
      ▼
Paandarin ang tamang servo
```

## 8. Aktuwal na dispensing at IR confirmation

Sa `STATE_DISPENSING`:

1. Ipinapadala ng `setServoDuty()` ang PWM push signal sa servo ng napiling product.
2. Nagsisimula ang `DISPENSE_TIMEOUT_MS` timer (default: 5 segundo).
3. Binabantayan ng firmware ang lahat ng apat na IR drop sensors.
4. Kapag kahit alin sa IR sensors ang naka-detect ng object, titigil agad ang lahat ng servos.
5. Saka lamang ibabawas ang product price sa credits at isang item sa stocks.
6. Tatawagin ang `saveProducts()` upang maisulat ang bagong stock sa `/products.json`.
7. Magpapakita ang LCD ng successful delivery at natitirang credits sa loob ng 2.5 segundo bago bumalik sa idle o waiting state.

Ito ang mahalagang safety rule: **hindi binabawasan ang credits at stock hangga't walang successful IR detection.** Dahil kahit alin sa apat na IR sensor ang puwedeng mag-confirm ng dispense, siguraduhing walang object sa harap ng alinman sa mga sensor bago magsimula ang transaction upang maiwasan ang false successful detection.

Kung walang IR sensor na mag-trigger bago matapos ang timeout, hihinto ang lahat ng servos, mapupunta ang machine sa `STATE_FAILED`, at magpapakita ang LCD ng dispense error. Walang credit o stock na mababawas.

## 9. Buong daloy sa isang tingin

```text
Boot ESP32
  │
  ├── Mount LittleFS; load products, pins, at account
  ├── Start 20x4 LCD, sensors, buttons, at servos
  ├── Start WiFi hotspot at web server
  │
  ├── Admin: login → manage products/pins sa browser
  │
  └── Customer: insert bottle → +1 credit → pindot button
                                      │
                                      ▼
                         Check stock at required credits
                                      │
                                      ▼
                           Run selected product servo
                                      │
                 Any IR sensor detects object? ── Hindi/timeout → error
                                      │ Oo
                                      ▼
                      Stop servo → deduct credit/stock → save products
                                      │
                                      ▼
                           Show success on 20x4 LCD
```

## 10. Mga file na pangunahing responsable

| File | Responsibilidad |
|---|---|
| `ArduinoSmartVendingMachine.ino` | Startup sequence at paulit-ulit na machine update |
| `wifi_manager.cpp` | ESP32 hotspot at local IP |
| `api.cpp` | REST API, authentication checks, at static web files |
| `auth.cpp` | NVS credentials at session tokens |
| `products.cpp` | Product list at `/products.json` persistence |
| `vending_controller.cpp` | Credits, state machine, buttons, servos, at IR logic |
| `display_manager.cpp` | 20x4 PCF8574 I2C LCD messages |
| `pin_config.cpp` | Configurable GPIO pins sa `/pins.json` |
| `data/js/dashboard.js` | Pagkuha at pag-render ng dashboard statistics |
| `data/js/products.js` | Product CRUD requests mula sa web page |

## 11. Mahahalagang paalala sa hardware

- Ang 20x4 PCF8574 LCD ay gumagamit ng GND, VCC, SDA, at SCL. Default: SDA = GPIO 23 (`P23`) at SCL = GPIO 22 (`P22`).
- Sa ESP32 38-pin board, ang `Pxx` ay GPIO xx; `VP` ay GPIO 36 at `VN` ay GPIO 39. Huwag gamitin ang `SD0`, `SD1`, `SD2`, `SD3`, `CMD`, at `CLK` dahil para ang mga ito sa flash memory.
- Kapag 5 V ang LCD backpack VCC, protektahan ang ESP32 gamit ang bidirectional I2C level shifter o 3.3 V pull-ups sa SDA/SCL. Hindi 5 V tolerant ang ESP32 GPIOs.
- Gumamit ng hiwalay na 5 V supply para sa servos at ikonekta ang ground nito sa ESP32 ground.
- I-check ang alignment at logic level ng IR sensors dahil dito nakasalalay ang confirmation bago bawasan ang stock at credit.
