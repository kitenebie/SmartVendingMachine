// ============================================================
//  display_manager.cpp -- 20x4 I2C LCD Driver (PCF8574)
//  Self-contained HD44780 4-bit over I2C driver using Wire.h
// ============================================================
#include "display_manager.h"
#include "pin_config.h"
#include "config.h"
#include <Wire.h>

// PCF8574 bit mappings for standard LCD backpacks:
// P0 = RS (0=Command, 1=Data)
// P1 = RW (0=Write)
// P2 = EN (Enable pulse)
// P3 = Backlight (1=ON, 0=OFF)
// P4-P7 = D4-D7 (Data bits)

#define LCD_RS_BIT 0x01
#define LCD_RW_BIT 0x02
#define LCD_EN_BIT 0x04
#define LCD_BL_BIT 0x08

static uint8_t s_i2cAddr = LCD_I2C_ADDR;
static uint8_t s_backlight = LCD_BL_BIT;
static bool s_lcdFound = false;

// Line start DDRAM addresses for 20x4 LCD
static const uint8_t ROW_OFFSETS[] = { 0x00, 0x40, 0x14, 0x54 };

// ------------------------------------------------------------
// Low-level PCF8574 I2C transmission
// ------------------------------------------------------------
static void i2cWriteExpander(uint8_t data) {
    Wire.beginTransmission(s_i2cAddr);
    Wire.write(data | s_backlight);
    Wire.endTransmission();
}

static void pulseEnable(uint8_t data) {
    i2cWriteExpander(data | LCD_EN_BIT);
    delayMicroseconds(2);
    i2cWriteExpander(data & ~LCD_EN_BIT);
    delayMicroseconds(50);
}

static void write4Bits(uint8_t value) {
    i2cWriteExpander(value);
    pulseEnable(value);
}

static void send(uint8_t value, uint8_t mode) {
    uint8_t highNibble = (value & 0xF0) | mode;
    uint8_t lowNibble  = ((value << 4) & 0xF0) | mode;
    write4Bits(highNibble);
    write4Bits(lowNibble);
}

static void command(uint8_t value) {
    send(value, 0);
}

static void writeChar(uint8_t value) {
    send(value, LCD_RS_BIT);
}

static void setCursor(uint8_t col, uint8_t row) {
    if (row > 3) row = 3;
    if (col > 19) col = 19;
    command(0x80 | (ROW_OFFSETS[row] + col));
}

static void printString(const String& str) {
    for (size_t i = 0; i < str.length() && i < 20; i++) {
        writeChar((uint8_t)str[i]);
    }
}

static void printPadded(uint8_t row, const String& text) {
    setCursor(0, row);
    String line = text;
    while (line.length() < 20) line += " ";
    if (line.length() > 20) line = line.substring(0, 20);
    printString(line);
}

// ------------------------------------------------------------
// Public API Functions
// ------------------------------------------------------------
void initDisplay() {
    int sda = getPinConfig().lcdSda;
    int scl = getPinConfig().lcdScl;

    Serial.printf("[LCD] Initializing 20x4 LCD on SDA=%d, SCL=%d...\n", sda, scl);
    Wire.begin(sda, scl, 100000);

    // Try the configured address first, then the other common PCF8574 address.
    s_i2cAddr = LCD_I2C_ADDR;
    Wire.beginTransmission(s_i2cAddr);
    if (Wire.endTransmission() != 0) {
        s_i2cAddr = (LCD_I2C_ADDR == 0x27) ? 0x3F : 0x27;
        Wire.beginTransmission(s_i2cAddr);
        if (Wire.endTransmission() != 0) {
            Serial.println("[LCD] WARNING: No I2C LCD found at 0x27 or 0x3F!");
            s_lcdFound = false;
            return;
        }
    }

    s_lcdFound = true;
    Serial.printf("[LCD] Found I2C LCD at address 0x%02X\n", s_i2cAddr);

    delay(50);
    // HD44780 4-bit initialization sequence
    write4Bits(0x30);
    delay(5);
    write4Bits(0x30);
    delayMicroseconds(150);
    write4Bits(0x30);
    delayMicroseconds(150);
    write4Bits(0x20); // Switch to 4-bit mode
    delay(2);

    command(0x28); // 4-bit mode, 2/4 lines, 5x8 font
    delayMicroseconds(50);
    command(0x08); // Display off
    delayMicroseconds(50);
    command(0x01); // Clear display
    delay(3);
    command(0x06); // Entry mode: increment cursor
    delayMicroseconds(50);
    command(0x0C); // Display ON, cursor OFF, blink OFF
    delay(2);

    displayCredits(0);
}

void clearDisplay() {
    if (!s_lcdFound) return;
    command(0x01);
    delay(3);
}

void displayMessage(const String& l1, const String& l2, const String& l3, const String& l4) {
    if (!s_lcdFound) return;
    printPadded(0, l1);
    printPadded(1, l2);
    printPadded(2, l3);
    printPadded(3, l4);
}

void displayCredits(int credits) {
    if (!s_lcdFound) return;
    char credStr[21];
    snprintf(credStr, sizeof(credStr), " CREDITS: %02d BOTTLE", credits);

    displayMessage(
        "=== SMART VENDING ==",
        String(credStr),
        "[1]Water    [2]Juice",
        "[3]Soda     [4]Snack"
    );
}

void displayProductSelected(int productNum, const String& name, float price, int credits) {
    if (!s_lcdFound) return;
    char l1[21], l2[21], l3[21];
    snprintf(l1, sizeof(l1), "SELECT: P%d", productNum);
    snprintf(l2, sizeof(l2), "%-12s Prc:%d", name.substring(0, 12).c_str(), (int)price);
    snprintf(l3, sizeof(l3), "Your Credits: %d", credits);

    displayMessage(
        String(l1),
        String(l2),
        String(l3),
        "Checking stock..."
    );
}

void displaySale(int productNum, const String& name) {
    if (!s_lcdFound) return;
    char l2[21];
    snprintf(l2, sizeof(l2), "Item: P%d %-10s", productNum, name.substring(0, 10).c_str());

    displayMessage(
        "=== DISPENSING... ==",
        String(l2),
        "Please wait...",
        "Watch delivery chute"
    );
}

void displayDone(int remainingCredits) {
    if (!s_lcdFound) return;
    char l4[21];
    snprintf(l4, sizeof(l4), "Credits left: %d", remainingCredits);

    displayMessage(
        "=== SUCCESSFUL! === ",
        "Please take item!",
        "Thank you!",
        String(l4)
    );
}

void displayNoCredit(int requiredPrice, int currentCredits) {
    if (!s_lcdFound) return;
    char l3[21];
    snprintf(l3, sizeof(l3), "Need:%d | Have:%d", requiredPrice, currentCredits);

    displayMessage(
        "== NO ENOUGH CREDIT=",
        "Insufficient balance",
        String(l3),
        "Insert more bottles!"
    );
}

void displayEmpty(int productNum, const String& name) {
    if (!s_lcdFound) return;
    char l2[21];
    snprintf(l2, sizeof(l2), "P%d %-15s", productNum, name.substring(0, 15).c_str());

    displayMessage(
        "=== OUT OF STOCK! ==",
        String(l2),
        "Item is currently",
        "unavailable. Sorry!"
    );
}

void displayError(int productNum) {
    if (!s_lcdFound) return;
    char l2[21];
    snprintf(l2, sizeof(l2), "Product %d jammed", productNum);

    displayMessage(
        "== DISPENSE ERROR ==",
        String(l2),
        "Credit NOT deducted!",
        "Please contact staff"
    );
}
