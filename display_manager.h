#pragma once
// ============================================================
//  display_manager.h -- 7-Segment (TM1637) & Status Display
// ============================================================
#include <Arduino.h>

void initDisplay();
void displayCredits(int credits);
void displayProduct(int productNum);
void displaySale();
void displayDone();
void displayNoCredit();
void displayEmpty();
void displayError();
void clearDisplay();
