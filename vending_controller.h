#pragma once
// ============================================================
//  vending_controller.h -- Vending Machine Controller & State Machine
// ============================================================
#include <Arduino.h>

enum VendingState {
    STATE_IDLE,
    STATE_CREDIT_VALIDATION,
    STATE_WAITING_SELECTION,
    STATE_CHECKING_PRODUCT,
    STATE_DISPENSING,
    STATE_SUCCESS,
    STATE_FAILED,
    STATE_INSUFFICIENT_CREDIT,
    STATE_OUT_OF_STOCK
};

/** Initialize pins, sensors, servos, and display. */
void initVendingMachine();

/** Non-blocking periodic loop function to be called from loop(). */
void updateVendingMachine();

/** Returns current accumulated credits in machine. */
int getCurrentCredits();

/** Add manual credit (e.g. from Web Admin or coin/bottle). */
void addCredit(int amount = 1);

/** Reset machine credits. */
void resetCredits();

/** Get current vending machine state name. */
String getVendingStateName();

/** Trigger a dispense programmatically (e.g. from Web UI). */
bool triggerDispense(int productIndex);
