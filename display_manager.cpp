// ============================================================
//  display_manager.cpp -- Dynamic TM1637 7-Segment Display
// ============================================================
#include "display_manager.h"
#include "pin_config.h"

static const uint8_t DIGIT_PATTERNS[] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

static void tm1637Start() {
    int dio = getPinConfig().dispDio;
    int clk = getPinConfig().dispClk;
    pinMode(dio, OUTPUT);
    digitalWrite(dio, HIGH);
    digitalWrite(clk, HIGH);
    delayMicroseconds(5);
    digitalWrite(dio, LOW);
    delayMicroseconds(5);
    digitalWrite(clk, LOW);
    delayMicroseconds(5);
}

static void tm1637Stop() {
    int dio = getPinConfig().dispDio;
    int clk = getPinConfig().dispClk;
    pinMode(dio, OUTPUT);
    digitalWrite(clk, LOW);
    digitalWrite(dio, LOW);
    delayMicroseconds(5);
    digitalWrite(clk, HIGH);
    delayMicroseconds(5);
    digitalWrite(dio, HIGH);
    delayMicroseconds(5);
}

static bool tm1637WriteByte(uint8_t b) {
    int dio = getPinConfig().dispDio;
    int clk = getPinConfig().dispClk;
    pinMode(dio, OUTPUT);
    for (int i = 0; i < 8; i++) {
        digitalWrite(clk, LOW);
        digitalWrite(dio, (b & (1 << i)) ? HIGH : LOW);
        delayMicroseconds(5);
        digitalWrite(clk, HIGH);
        delayMicroseconds(5);
    }
    digitalWrite(clk, LOW);
    pinMode(dio, INPUT);
    delayMicroseconds(5);
    digitalWrite(clk, HIGH);
    delayMicroseconds(5);
    bool ack = (digitalRead(dio) == LOW);
    digitalWrite(clk, LOW);
    pinMode(dio, OUTPUT);
    return ack;
}

static void sendSegments(const uint8_t segments[4]) {
    tm1637Start();
    tm1637WriteByte(0x40);
    tm1637Stop();

    tm1637Start();
    tm1637WriteByte(0xC0);
    for (int i = 0; i < 4; i++) {
        tm1637WriteByte(segments[i]);
    }
    tm1637Stop();

    tm1637Start();
    tm1637WriteByte(0x88 | 0x07);
    tm1637Stop();
}

void initDisplay() {
    int clk = getPinConfig().dispClk;
    int dio = getPinConfig().dispDio;
    pinMode(clk, OUTPUT);
    pinMode(dio, OUTPUT);
    digitalWrite(clk, HIGH);
    digitalWrite(dio, HIGH);
    displayCredits(0);
}

void displayCredits(int credits) {
    if (credits < 0) credits = 0;
    if (credits > 99) credits = 99;
    uint8_t segs[4];
    segs[0] = 0x39; // 'C'
    segs[1] = 0x00; // ' '
    segs[2] = DIGIT_PATTERNS[(credits / 10) % 10];
    segs[3] = DIGIT_PATTERNS[credits % 10];
    sendSegments(segs);
}

void displayProduct(int productNum) {
    uint8_t segs[4] = { 0x73, 0x00, 0x00, DIGIT_PATTERNS[productNum % 10] }; // "P  #"
    sendSegments(segs);
}

void displaySale() {
    uint8_t segs[4] = { 0x6D, 0x77, 0x38, 0x79 }; // "SALE"
    sendSegments(segs);
}

void displayDone() {
    uint8_t segs[4] = { 0x5E, 0x3F, 0x54, 0x79 }; // "dOnE"
    sendSegments(segs);
}

void displayNoCredit() {
    uint8_t segs[4] = { 0x54, 0x3F, 0x39, 0x50 }; // "nOCr"
    sendSegments(segs);
}

void displayEmpty() {
    uint8_t segs[4] = { 0x79, 0x54, 0x73, 0x78 }; // "EnPt"
    sendSegments(segs);
}

void displayError() {
    uint8_t segs[4] = { 0x79, 0x50, 0x50, 0x00 }; // "Err "
    sendSegments(segs);
}

void clearDisplay() {
    uint8_t segs[4] = { 0x00, 0x00, 0x00, 0x00 };
    sendSegments(segs);
}
