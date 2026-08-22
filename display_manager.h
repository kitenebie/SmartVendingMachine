#pragma once
// ============================================================
//  display_manager.h -- 20x4 I2C LCD Display (PCF8574 Backpack)
// ============================================================
#include <Arduino.h>

/** Initialize the 20x4 I2C LCD display using configured SDA/SCL pins. */
void initDisplay();

/** Display default idle screen showing current credits and menu. */
void displayCredits(int credits);

/** Display product selected confirmation & price check. */
void displayProductSelected(int productNum, const String& name, float price, int credits);

/** Display dispensing in-progress screen. */
void displaySale(int productNum, const String& name);

/** Display successful delivery screen with remaining credits. */
void displayDone(int remainingCredits);

/** Display insufficient credits warning. */
void displayNoCredit(int requiredPrice, int currentCredits);

/** Display out-of-stock warning. */
void displayEmpty(int productNum, const String& name);

/** Display dispensing timeout / motor error screen. */
void displayError(int productNum);

/** Clear all 4 lines of the LCD. */
void clearDisplay();

/** Print custom 4-line text directly to LCD. */
void displayMessage(const String& l1, const String& l2 = "", const String& l3 = "", const String& l4 = "");
