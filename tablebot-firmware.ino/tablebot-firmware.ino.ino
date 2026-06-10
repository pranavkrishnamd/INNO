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

BotState bot;

#define NTP_SERVER   "pool.ntp.org"
#define GMT_OFFSET   19800
#define DAYLIGHT     0

#define INTERVAL_BLE          20
#define INTERVAL_INPUT        50
#define INTERVAL_ANIM         300 // face animation
#define INTERVAL_GAME         20     // game tick
#define INTERVAL_CLOCK        1000
#define INTERVAL_IDLE_CHECK   5000
#define INTERVAL_STATUS       5000
#define INTERVAL_WEATHER      600000
#define INTERVAL_NTP          3600000
#define INTERVAL_BATTERY      30000
#define INTERVAL_SERVO        50
#define INTERVAL_RANDOM_LOOK  8000

unsigned long lastBLECheck     = 0;
unsigned long lastInputCheck   = 0;
unsigned long lastAnimFrame    = 0;
unsigned long lastClockUpdate  = 0;
unsigned long lastIdleCheck    = 0;
unsigned long lastStatusSend   = 0;
unsigned long lastWeatherFetch = 0;
unsigned long lastNTPSync      = 0;
unsigned long lastBatteryCheck = 0;
unsigned long lastRandomLook   = 0;
unsigned long lastGameFrame    = 0;

bool sensor1WasHigh = false;
bool sensor2WasHigh = false;

void checkBattery();
const char* getDayName(int dayNum);

void setup() {
  Serial.begin(115200);
WiFiManager wifiManager;
   // Reset WiFi credentials — remove after reset
  wifiManager.resetSettings();
  Serial.println("WiFi: Credentials cleared!");
  
  // rest of your setup...
  Serial.println("==============================");
  Serial.println("TableBot booting...");

  // LEDs
  pinMode(PIN_LED_BLUE, OUTPUT);
  pinMode(PIN_LED_RED,  OUTPUT);
  digitalWrite(PIN_LED_BLUE, LOW);
  digitalWrite(PIN_LED_RED,  LOW);

  // Touch sensors
  pinMode(PIN_TOUCH,      INPUT);
  pinMode(PIN_TOUCH_GAME, INPUT);

  // Charging pins
  pinMode(PIN_CHARGE_STATUS, INPUT_PULLUP);
  pinMode(PIN_CHARGE_FULL,   INPUT_PULLUP);

  // Amp
  pinMode(PIN_AMP_SD_MODE, OUTPUT);
  digitalWrite(PIN_AMP_SD_MODE, HIGH);

  // OLED
  Display_init();

// WiFi
Serial.println("WiFi: Connecting...");
wifiManager.autoConnect("TableBot-Setup");
bot.wifiConnected = true;
Serial.print("WiFi: Connected → ");
Serial.println(WiFi.localIP());

// NTP
configTime(GMT_OFFSET, DAYLIGHT, NTP_SERVER);
struct tm timeInfo;
 int attempts = 0;
while (!getLocalTime(&timeInfo) && attempts < 10) {
  Serial.println("NTP: Waiting...");
  delay(500);
  attempts++;
}
if (attempts < 10) {
  bot.ntpSynced = true;
  bot.time.hour   = timeInfo.tm_hour;
  bot.time.minute = timeInfo.tm_min;
  bot.time.second = timeInfo.tm_sec;
  Serial.println("NTP: Synced");
} else {
  bot.time.hour   = 10;
  bot.time.minute = 30;
  bot.time.second = 0;
  Serial.println("NTP: Failed — using 10:30");
}

 // Weather — first fetch
bot.weather.latitude  = 8.8932;
bot.weather.longitude = 76.6141;
bool weatherOk = Weather_fetch(bot);
if (weatherOk) Serial.println("Weather: Done");
else Serial.println("Weather: Failed");

  // BLE
  BLE_init(bot);

  // Animation and game
  Animation_init();
  Game_init();

  // State and mode
  State_init(bot);
  Mode_init(bot);

  bot.firstBootDone = true;
  Serial.println("TableBot: Boot complete → IDLE");
  Serial.println("==============================");
}

void loop() {
  unsigned long now = millis();

  // 20ms: BLE
  if (now - lastBLECheck >= INTERVAL_BLE) {
    lastBLECheck = now;
    BLE_poll(bot);
    digitalWrite(PIN_LED_BLUE,
                 bot.ble.isConnected ? HIGH : LOW);
    bot.led.blueOn = bot.ble.isConnected;
    if (bot.ble.newCommandReceived) {
      Mode_apply(bot);
    }
  }

  // 50ms: Input
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

  // 20ms: Animation and game
// Animation — 100ms
if (now - lastAnimFrame >= INTERVAL_ANIM) {
  lastAnimFrame = now;
  if (bot.mode != MODE_GAME &&
      bot.state != STATE_SLEEPING) {
    Anim_tick(bot.face);
    Display_render(bot);
  }
}

// Game — separate 20ms
if (now - lastGameFrame >= INTERVAL_GAME) {
  lastGameFrame = now;
  if (bot.mode == MODE_GAME &&
      bot.game.isRunning) {
    Game_tick(bot);
  }
}

  // 1s: Clock
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

  // 5s: Idle check
  if (now - lastIdleCheck >= INTERVAL_IDLE_CHECK) {
    lastIdleCheck = now;
    State_checkIdle(bot);
  }

  // 5s: BLE status
  if (now - lastStatusSend >= INTERVAL_STATUS) {
    lastStatusSend = now;
    if (bot.ble.isConnected) {
      BLE_sendStatus(bot);
    }
  }

  // 30s: Battery
  if (now - lastBatteryCheck >= INTERVAL_BATTERY) {
    lastBatteryCheck = now;
    checkBattery();
  }
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
  digitalWrite(PIN_LED_RED,
               bot.battery.isCharging ? HIGH : LOW);
  bot.led.redOn = bot.battery.isCharging;
  bot.display.showBatteryIcon = bot.battery.isLow;
  if (percent < 10 && bot.ble.isConnected) {
    BLE_sendMessage("ALERT:BATTERY_CRITICAL");
  }
  Serial.print("Battery: ");
  Serial.print(percent);
  Serial.print("%  Charging: ");
  Serial.println(bot.battery.isCharging ? "YES" : "NO");
}

const char* getDayName(int dayNum) {
  const char* days[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday"
  };
  if (dayNum >= 0 && dayNum <= 6)
    return days[dayNum];
  return "Unknown";
}