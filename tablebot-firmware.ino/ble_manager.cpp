// ============================================
// ble_manager.cpp — TableBot
// The actual BLE communication code.
// Phone connects here, sends commands here,
// receives status updates from here.
// Written by: Ribin
// ============================================

#include "ble_manager.h"
#include "bot_state.h"
#include <NimBLEDevice.h>

// ── BLE Identity ─────────────────────────────
// This is the name your phone sees when scanning
#define BLE_DEVICE_NAME     "TableBot"

// UUIDs — unique IDs for our BLE service
// Think of these like a phone number for BLE
// Don't change these — phone app uses same IDs
#define SERVICE_UUID        "12345678-1234-1234-1234-123456789abc"
#define CMD_CHAR_UUID       "12345678-1234-1234-1234-123456789abd"  // phone → bot
#define STATUS_CHAR_UUID    "12345678-1234-1234-1234-123456789abe"  // bot → phone

// ── Internal BLE objects ──────────────────────
// These are the building blocks of a BLE server
// You don't need to fully understand these yet
static NimBLEServer*         pServer      = nullptr;
static NimBLECharacteristic* pCmdChar     = nullptr;  // receives commands
static NimBLECharacteristic* pStatusChar  = nullptr;  // sends status
static bool                  deviceConnected = false;

// ── Connection Callbacks ─────────────────────
// These functions run automatically when
// phone connects or disconnects
class BotServerCallbacks : public NimBLEServerCallbacks {

  // Runs when phone successfully connects
  void onConnect(NimBLEServer* pServer) {
    deviceConnected = true;
    Serial.println("BLE Manager: Phone connected");
  }

  // Runs when phone disconnects
  void onDisconnect(NimBLEServer* pServer) {
    deviceConnected = false;
    // Start advertising again so phone can reconnect
    NimBLEDevice::startAdvertising();
    Serial.println("BLE Manager: Phone disconnected — advertising again");
  }
};

// ── Command Callbacks ─────────────────────────
// Runs automatically when phone sends a command
class CommandCallbacks : public NimBLECharacteristicCallbacks {

  void onWrite(NimBLECharacteristic* pCharacteristic) {
    // Read what the phone sent
    std::string rawValue = pCharacteristic->getValue();

    if (rawValue.length() > 0) {
      // Copy it into our bot struct
      // so main loop can read it
      extern BotState bot;
      strncpy(bot.ble.lastCommand,
              rawValue.c_str(),
              sizeof(bot.ble.lastCommand) - 1);

      // Set flag so main loop knows
      // a new command is waiting
      bot.ble.newCommandReceived = true;

      Serial.print("BLE Manager: Command received → ");
      Serial.println(bot.ble.lastCommand);
    }
  }
};

// ── BLE_init ─────────────────────────────────
// Call once in setup()
// Sets up the BLE server and starts advertising
void BLE_init(BotState& bot) {
  Serial.println("BLE Manager: Starting...");

  // Initialize BLE with device name
  NimBLEDevice::init(BLE_DEVICE_NAME);

  // Create the server
  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new BotServerCallbacks());

  // Create the service
  NimBLEService* pService = pServer->createService(SERVICE_UUID);

  // Create command characteristic
  // Phone writes commands here
  pCmdChar = pService->createCharacteristic(
    CMD_CHAR_UUID,
    NIMBLE_PROPERTY::WRITE
  );
  pCmdChar->setCallbacks(new CommandCallbacks());

  // Create status characteristic
  // Bot writes status here, phone reads it
  pStatusChar = pService->createCharacteristic(
    STATUS_CHAR_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
  );

  // Start the service
  pService->start();

  // Start advertising so phones can find us
  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();

  // Update bot struct
  bot.ble.isConnected         = false;
  bot.ble.newCommandReceived  = false;

  Serial.println("BLE Manager: Ready — advertising as 'TableBot'");
}

// ── BLE_poll ─────────────────────────────────
// Call every 20ms in main loop
// Updates connection status in bot struct
void BLE_poll(BotState& bot) {
  // Update connection status
  bot.ble.isConnected        = deviceConnected;
  bot.display.showBLEIcon    = deviceConnected;
}

// ── BLE_sendStatus ───────────────────────────
// Sends current bot status to connected phone
// Phone app can display this information
void BLE_sendStatus(BotState& bot) {
  if (!deviceConnected) return;

  // Build a status string with key info
  char statusMsg[128];
  snprintf(statusMsg, sizeof(statusMsg),
    "STATE:%d|MODE:%d|BAT:%d|TEMP:%.1f|WIFI:%d",
    bot.state,
    bot.mode,
    bot.battery.percentage,
    bot.weather.temp,
    bot.wifiConnected ? 1 : 0
  );

  // Send it to phone
  pStatusChar->setValue(statusMsg);
  pStatusChar->notify();

  Serial.print("BLE Manager: Status sent → ");
  Serial.println(statusMsg);
}

// ── BLE_sendMessage ──────────────────────────
// Sends a simple text message to phone
// Use for alerts like "ERROR:WIFI_FAILED"
void BLE_sendMessage(const char* message) {
  if (!deviceConnected) return;

  pStatusChar->setValue(message);
  pStatusChar->notify();

  Serial.print("BLE Manager: Message sent → ");
  Serial.println(message);
}

// ── BLE_isConnected ──────────────────────────
// Simple check — is phone connected right now?
bool BLE_isConnected() {
  return deviceConnected;
}