// ============================================
// tablebot-firmware.ino — INNO TableBot
// Board: Waveshare ESP32-S3-Tiny
// Stage 2 — Updated June 2026
// ============================================

#include <Wire.h>
#include <Arduino.h>
#include "pins.h"
#include "bot_state.h"
#include "state_manager.h"
#include "ble_manager.h"
#include "mode_manager.h"
#include "display_manager.h"
#include "animation_engine.h"
#include "game_engine.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <Adafruit_NeoPixel.h>
#include "time.h"

// ── Global instances ──────────────────────────
BotState bot;
Adafruit_NeoPixel ws2812(WS2812_COUNT,
                          PIN_WS2812,
                          NEO_GRB + NEO_KHZ800);

// ── NTP ───────────────────────────────────────
#define NTP_SERVER   "pool.ntp.org"
#define GMT_OFFSET   19800
#define DAYLIGHT     0

// ── Task intervals ────────────────────────────
#define INTERVAL_BLE          20
#define INTERVAL_INPUT        50
#define INTERVAL_ANIM         100
#define INTERVAL_GAME         20
#define INTERVAL_CLOCK        1000
#define INTERVAL_IDLE_CHECK   5000
#define INTERVAL_STATUS       5000
#define INTERVAL_WEATHER      600000
#define INTERVAL_NTP          3600000
#define INTERVAL_BATTERY      30000

// ── Timing variables ─────────────────────────
unsigned long lastBLECheck     = 0;
unsigned long lastInputCheck   = 0;
unsigned long lastAnimFrame    = 0;
unsigned long lastGameFrame    = 0;
unsigned long lastClockUpdate  = 0;
unsigned long lastIdleCheck    = 0;
unsigned long lastStatusSend   = 0;
unsigned long lastWeatherFetch = 0;
unsigned long lastNTPSync      = 0;
unsigned long lastBatteryCheck = 0;

bool sensor1WasHigh = false;
bool sensor2WasHigh = false;

void checkBattery();
void setRGB(uint8_t r, uint8_t g, uint8_t b);
void setWS2812(uint8_t r, uint8_t g, uint8_t b);
const char* getDayName(int d);

// ════════════════════════════════════════════
// SETUP
// ════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  Serial.println("==============================");
  Serial.println("INNO TableBot booting...");

  // ── Touch sensors ────────────────────────
  pinMode(PIN_TOUCH,      INPUT_PULLDOWN);
  pinMode(PIN_TOUCH_GAME, INPUT_PULLDOWN);

  // ── Charging pins ────────────────────────
  pinMode(PIN_CHARGE_STATUS, INPUT_PULLUP);
  pinMode(PIN_CHARGE_FULL,   INPUT_PULLUP);

  // ── Amp ──────────────────────────────────
  // SD_MODE hardwired to 3.3V — no pinMode needed

  // ── RGB LED ──────────────────────────────
  pinMode(PIN_RGB_R, OUTPUT);
  pinMode(PIN_RGB_G, OUTPUT);
  pinMode(PIN_RGB_B, OUTPUT);
  setRGB(0, 0, 0);  // off at boot

  // ── WS2812 onboard LED ───────────────────
  ws2812.begin();
  ws2812.clear();
  ws2812.show();

  // ── Display ──────────────────────────────
  Display_init();

  // ── BLE FIRST ────────────────────────────
  BLE_init(bot);

  // ── WiFi SECOND ──────────────────────────
  Serial.println("WiFi: Connecting...");
  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(120);
  wifiManager.autoConnect("INNO-Setup");
  bot.wifiConnected = true;
  Serial.print("WiFi: Connected → ");
  Serial.println(WiFi.localIP());

  // ── NTP ──────────────────────────────────
  configTime(GMT_OFFSET, DAYLIGHT, NTP_SERVER);
  struct tm timeInfo;
  int attempts = 0;
  while (!getLocalTime(&timeInfo) && attempts < 10) {
    Serial.println("NTP: Waiting...");
    delay(500);
    attempts++;
  }
  if (attempts < 10) {
    bot.ntpSynced   = true;
    bot.time.hour   = timeInfo.tm_hour;
    bot.time.minute = timeInfo.tm_min;
    bot.time.second = timeInfo.tm_sec;
    Serial.println("NTP: Synced");
  } else {
    bot.time.hour   = 0;
    bot.time.minute = 0;
    bot.time.second = 0;
    Serial.println("NTP: Failed");
  }

  // ── Weather ──────────────────────────────
  bot.weather.latitude  = 8.8932;
  bot.weather.longitude = 76.6141;
  bool weatherOk = Weather_fetch(bot);
  Serial.println(weatherOk ?
    "Weather: Done" : "Weather: Failed");

  // ── Animation + Game ─────────────────────
  Animation_init();
  Game_init();

  // ── State + Mode ─────────────────────────
  State_init(bot);
  Mode_init(bot);

  bot.firstBootDone = true;
  Serial.println("INNO: Boot complete");
  Serial.println("==============================");
}

// ════════════════════════════════════════════
// LOOP
// ════════════════════════════════════════════
void loop() {
  unsigned long now = millis();

  // ── 20ms: BLE ────────────────────────────
  if (now - lastBLECheck >= INTERVAL_BLE) {
    lastBLECheck = now;
    BLE_poll(bot);

    // WS2812 = BLE indicator
    if (bot.ble.isConnected) {
      setWS2812(0, 0, 50);  // dim blue
    } else {
      setWS2812(0, 0, 0);   // off
    }

    if (bot.ble.newCommandReceived) {
      Mode_apply(bot);
    }
  }

  // ── 50ms: Input ──────────────────────────
  if (now - lastInputCheck >= INTERVAL_INPUT) {
    lastInputCheck = now;

    bool s1High = (digitalRead(PIN_TOUCH) == HIGH);
    if (s1High && !sensor1WasHigh) {
      if (now - bot.lastTouchMs > 300) {
        bot.lastTouchMs = now;
        State_onTouch(bot);
      }
    }
    sensor1WasHigh = s1High;

    bool s2High = (digitalRead(PIN_TOUCH_GAME) == HIGH);
    if (s2High && !sensor2WasHigh) {
      bot.lastTouchMs = now;
      if (bot.mode == MODE_GAME) {
        Mode_exitGame(bot);
      } else {
        Mode_setGame(bot);
      }
    }
    sensor2WasHigh = s2High;
  }

  // ── 100ms: Animation ─────────────────────
  if (now - lastAnimFrame >= INTERVAL_ANIM) {
    lastAnimFrame = now;
    if (bot.mode != MODE_GAME &&
        bot.state != STATE_SLEEPING) {
      Anim_tick(bot.face);
      Display_render(bot);
    }
  }

  // ── 20ms: Game ───────────────────────────
  if (now - lastGameFrame >= INTERVAL_GAME) {
    lastGameFrame = now;
    if (bot.mode == MODE_GAME &&
        bot.game.isRunning) {
      Game_tick(bot);
    }
  }

  // ── 1s: Clock ────────────────────────────
  if (now - lastClockUpdate >= INTERVAL_CLOCK) {
    lastClockUpdate = now;
    if (bot.mode != MODE_GAME) {
      bot.time.second++;
      if (bot.time.second >= 60) {
        bot.time.second = 0;
        bot.time.minute++;
        if (bot.time.minute >= 60) {
          bot.time.minute = 0;
          bot.time.hour++;
          if (bot.time.hour >= 24)
            bot.time.hour = 0;
        }
      }
    }
  }

  // ── 5s: Idle check ───────────────────────
  if (now - lastIdleCheck >= INTERVAL_IDLE_CHECK) {
    lastIdleCheck = now;
    State_checkIdle(bot);
  }

  // ── 5s: BLE status ───────────────────────
  if (now - lastStatusSend >= INTERVAL_STATUS) {
    lastStatusSend = now;
    if (bot.ble.isConnected) {
      BLE_sendStatus(bot);
    }
  }

  // ── 30s: Battery ─────────────────────────
  if (now - lastBatteryCheck >= INTERVAL_BATTERY) {
    lastBatteryCheck = now;
    checkBattery();
  }

  // ── 10min: Weather ───────────────────────
  if (now - lastWeatherFetch >= INTERVAL_WEATHER) {
    lastWeatherFetch = now;
    if (bot.wifiConnected &&
        bot.mode != MODE_GAME) {
      State_onUpdateStart(bot);
      bool ok = Weather_fetch(bot);
      if (ok) State_onUpdateDone(bot);
      else    State_onError(bot);
    }
  }

  // ── 1hr: NTP resync ──────────────────────
  if (now - lastNTPSync >= INTERVAL_NTP) {
    lastNTPSync = now;
    configTime(GMT_OFFSET, DAYLIGHT, NTP_SERVER);
    Serial.println("NTP: Re-synced");
  }
}

// ════════════════════════════════════════════
// HELPERS
// ════════════════════════════════════════════

// RGB LED — common anode (LOW = ON)
void setRGB(uint8_t r, uint8_t g, uint8_t b) {
  digitalWrite(PIN_RGB_R, r > 0 ? LOW : HIGH);
  digitalWrite(PIN_RGB_G, g > 0 ? LOW : HIGH);
  digitalWrite(PIN_RGB_B, b > 0 ? LOW : HIGH);
}

// WS2812 onboard LED
void setWS2812(uint8_t r, uint8_t g, uint8_t b) {
  ws2812.setPixelColor(0, ws2812.Color(r, g, b));
  ws2812.show();
}

void checkBattery() {
  int raw     = analogRead(PIN_BATTERY_ADC);
  int percent = constrain(
                  map(raw, 0, 4095, 0, 100),
                  0, 100);

  bot.battery.percentage = percent;
  bot.battery.isCharging =
    (digitalRead(PIN_CHARGE_STATUS) == LOW);
  bot.battery.isLow = (percent < 20);

  // RGB LED battery indication
  if (bot.battery.isCharging) {
    setRGB(255, 0, 0);      // red = charging
  } else if (percent > 60) {
    setRGB(0, 255, 0);      // green = good
  } else if (percent > 20) {
    setRGB(255, 165, 0);    // orange = medium
  } else {
    setRGB(255, 0, 0);      // red = low
  }

  bot.display.showBatteryIcon = bot.battery.isLow;

  if (percent < 10 && bot.ble.isConnected) {
    BLE_sendMessage("BATTERY:CRITICAL");
  }

  Serial.print("Battery: ");
  Serial.print(percent);
  Serial.print("%  Charging: ");
  Serial.println(bot.battery.isCharging ?
                 "YES" : "NO");
}

const char* getDayName(int d) {
  const char* days[] = {
    "Sunday","Monday","Tuesday","Wednesday",
    "Thursday","Friday","Saturday"
  };
  if (d >= 0 && d <= 6) return days[d];
  return "Unknown";
}