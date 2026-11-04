# Graph Report - ArduinoSmartVendingMachine  (2026-10-16)

## Corpus Check
- 25 files · ~112,018 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 1 file(s) not represented in the graph (top: .ino 1)

## Summary
- 343 nodes · 671 edges · 19 communities
- Extraction: 89% EXTRACTED · 11% INFERRED · 0% AMBIGUOUS · INFERRED: 75 edges (avg confidence: 0.85)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `7d70eb32`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- auth.cpp
- display_manager.cpp
- api.cpp
- Paano Gumagana ang Vending Machine: Mula Web Server Hanggang Dispensing
- ESP32 Vending Machine Plan
- PendingDispense
- products.cpp
- 🛠️ Complete Wiring & Assembly Guide — ESP32 Smart Vending Machine (4 Products + 20x4 I2C LCD)
- wifi_manager.cpp
- 35. Development Phases
- 30. Module Responsibilities
- 3. Hardware Components
- Ponytail, lazy senior dev mode
- Ponytail
- auth.js
- 4. Bottle Credit Detection
- 13. Credit Deduction Rule
- 15. Dispensing Timeout
- 24. Sensor Debouncing

## God Nodes (most connected - your core abstractions)
1. `ESP32 Vending Machine Plan` - 38 edges
2. `User` - 30 edges
3. `sendJson()` - 22 edges
4. `sendError()` - 20 edges
5. `updateReverseVending()` - 16 edges
6. `requireUser()` - 15 edges
7. `PendingDispense` - 14 edges
8. `requireAdmin()` - 13 edges
9. `readBody()` - 12 edges
10. `Paano Gumagana ang Vending Machine: Mula Web Server Hanggang Dispensing` - 12 edges

## Surprising Connections (you probably didn't know these)
- `8. Aktuwal na dispensing at IR confirmation` --references--> `saveProducts()`  [INFERRED]
  PaanoGumaganaAngVendingMachineFromWebserverUpToDispensing.md → products.cpp
- `1. Ano ang nangyayari sa pag-on ng ESP32` --references--> `updateVendingMachine()`  [INFERRED]
  PaanoGumaganaAngVendingMachineFromWebserverUpToDispensing.md → vending_controller.cpp
- `requireUser()` --calls--> `expireInactiveCredits()`  [INFERRED]
  api.cpp → auth.cpp
- `requireUser()` --calls--> `sessionUser()`  [INFERRED]
  api.cpp → auth.cpp
- `userJson()` --calls--> `getDailyDispenseCount()`  [INFERRED]
  api.cpp → auth.cpp

## Import Cycles
- None detected.

## Communities (19 total, 0 thin omitted)

### Community 0 - "auth.cpp"
Cohesion: 0.08
Nodes (46): beginRedemption(), commitDelivered(), recoverRedemption(), addUserCredits(), appendDispenseLog(), changeUserPassword(), createSession(), currentCreditActivity() (+38 more)

### Community 1 - "display_manager.cpp"
Cohesion: 0.09
Nodes (39): updateReverseVending(), clearDisplay(), command(), displayCredits(), displayDone(), displayEmpty(), displayError(), displayMessage() (+31 more)

### Community 2 - "api.cpp"
Cohesion: 0.15
Nodes (32): ActiveSession, expiresAt, locked, userId, ApiLock, emptyBody(), handleAddProduct(), handleAdminCredits() (+24 more)

### Community 3 - "Paano Gumagana ang Vending Machine: Mula Web Server Hanggang Dispensing"
Cohesion: 0.06
Nodes (30): 1. Default Pin Mapping Table (4-Product System + 20x4 I2C LCD), 2. 20x4 I2C LCD Module (PCF8574) Connections, 3. Power Architecture & Wiring Diagram, 4. Sensor Voltage Safety (Level Shifting), 5. Hardware Assembly & Connection Procedures, 6. 20x4 LCD Screen Layouts, Common Ground Rule:, ESP32 Smart Vending Machine & Inventory System — Pin Connection & Setup Manual (+22 more)

### Community 4 - "ESP32 Vending Machine Plan"
Cohesion: 0.06
Nodes (31): 10. Servo Dispensing Mechanism, 11. IR Delivery Confirmation, 12. Stop Servo After Successful Detection, 14. Stock Deduction Rule, 16. Failed Dispense, 17. Transaction State Machine, 18. 20x4 I2C LCD Display, 19. Complete Transaction Example (+23 more)

### Community 5 - "PendingDispense"
Cohesion: 0.09
Nodes (19): PendingDispense, delivered, journalWritten, productId, slot, targetCreditActivity, targetCreditActivityAt, targetCredits (+11 more)

### Community 6 - "products.cpp"
Cohesion: 0.18
Nodes (14): addProduct(), deleteProduct(), getProductById(), loadProducts(), Product, creditCost, id, name (+6 more)

### Community 7 - "🛠️ Complete Wiring & Assembly Guide — ESP32 Smart Vending Machine (4 Products + 20x4 I2C LCD)"
Cohesion: 0.12
Nodes (17): 1. Bill of Materials (Hardware Checklist), 2. Master Pinout & Wire Color Table, 3. 20x4 I2C LCD Display (PCF8574) Wiring, 4. Power Supply Architecture & Common Ground, 5. Sensor Voltage Safety (Level Shifter / Divider), 6. Step-by-Step Assembly Instructions, 7. 20x4 LCD Screen Layout Reference, 8. Testing & Calibration Procedures (+9 more)

### Community 8 - "wifi_manager.cpp"
Cohesion: 0.17
Nodes (8): handleAdminWiFi(), connectToStation(), getLocalIP(), loadAccessPointPassword(), loadStationCredentials(), saveStationCredentials(), stationConnected(), timeSynchronized()

### Community 9 - "35. Development Phases"
Cohesion: 0.22
Nodes (9): 35. Development Phases, Phase 1 — Hardware Test, Phase 2 — Bottle Detection, Phase 3 — Product Selection, Phase 4 — Servo Control, Phase 5 — Product Detection, Phase 6 — Transaction Logic, Phase 7 — Display (+1 more)

### Community 10 - "30. Module Responsibilities"
Cohesion: 0.25
Nodes (8): 30. Module Responsibilities, Button Manager, Credit Manager, Display Manager, Product Manager, Sensor Manager, Servo Manager, Vending Controller

### Community 11 - "3. Hardware Components"
Cohesion: 0.29
Nodes (7): 3. Hardware Components, Bottle Detection, Main Controller, Product Delivery Confirmation, Product Dispensing, Product Selection, User Feedback

### Community 12 - "Ponytail, lazy senior dev mode"
Cohesion: 0.40
Nodes (4): Before you write, Ponytail, lazy senior dev mode, Ponytail project rules, The smallest complete change

### Community 13 - "Ponytail"
Cohesion: 0.40
Nodes (4): Before you write, Levels, Ponytail, The smallest complete change

### Community 14 - "auth.js"
Cohesion: 0.83
Nodes (3): api(), logout(), token()

### Community 15 - "4. Bottle Credit Detection"
Cohesion: 0.50
Nodes (4): 4. Bottle Credit Detection, Invalid, Requirement, Valid

### Community 16 - "13. Credit Deduction Rule"
Cohesion: 0.67
Nodes (3): 13. Credit Deduction Rule, Correct, Incorrect

### Community 17 - "15. Dispensing Timeout"
Cohesion: 0.67
Nodes (3): 15. Dispensing Timeout, NO, YES

### Community 18 - "24. Sensor Debouncing"
Cohesion: 0.67
Nodes (3): 24. Sensor Debouncing, Bottle Sensors, Product IR Sensor

## Knowledge Gaps
- **36 isolated node(s):** `userId`, `locked`, `expiresAt`, `userId`, `productId` (+31 more)
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 157 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Paano Gumagana ang Vending Machine: Mula Web Server Hanggang Dispensing` connect `Paano Gumagana ang Vending Machine: Mula Web Server Hanggang Dispensing` to `display_manager.cpp`?**
  _High betweenness centrality (0.443) - this node is a cross-community bridge._
- **What connects `userId`, `locked`, `expiresAt` to the rest of the system?**
  _36 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `auth.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.08455625436757512 - nodes in this community are weakly interconnected._
- **Why does `saveProducts()` connect `products.cpp` to `auth.cpp`, `Paano Gumagana ang Vending Machine: Mula Web Server Hanggang Dispensing`, `PendingDispense`?**
  _High betweenness centrality (0.346) - this node is a cross-community bridge._
- **Should `display_manager.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.08653061224489796 - nodes in this community are weakly interconnected._
- **Why does `8. Aktuwal na dispensing at IR confirmation` connect `Paano Gumagana ang Vending Machine: Mula Web Server Hanggang Dispensing` to `products.cpp`?**
  _High betweenness centrality (0.342) - this node is a cross-community bridge._
- **Should `Paano Gumagana ang Vending Machine: Mula Web Server Hanggang Dispensing` be split into smaller, more focused modules?**
  _Cohesion score 0.05714285714285714 - nodes in this community are weakly interconnected._