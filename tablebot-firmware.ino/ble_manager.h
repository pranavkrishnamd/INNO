// ============================================
// ble_manager.h — TableBot
// Declares what the BLE manager can do.
// Handles all phone ↔ bot communication.
// Uses NimBLE library (lighter than ArduinoBLE)
// ============================================

#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include "bot_state.h"

// Call once in setup()
// Starts BLE server and begins advertising
// Phone can find and connect after this
void BLE_init(BotState& bot);

// Call every 20ms in main loop
// Checks if phone sent a new command
// If yes → saves to bot.ble.lastCommand
//        → sets bot.ble.newCommandReceived = true
void BLE_poll(BotState& bot);

// Call this to send current status to phone
// Sends battery, state, weather, mode
void BLE_sendStatus(BotState& bot);

// Call this to send a simple message to phone
// Example: BLE_sendMessage("ERROR:WIFI_FAILED")
void BLE_sendMessage(const char* message);

// Returns true if phone is currently connected
bool BLE_isConnected();

#endif