// ============================================
// display_manager.cpp — TableBot
// Homepage display — face, clock, weather
// Original code by Pranav
// Integrated by Ribin — May 2026
// Changes from original:
//   - Removed setup() and loop()
//   - Removed hardcoded WiFi credentials
//   - Removed connectWiFi() — main handles it
//   - Removed duplicate weather timer
//   - Removed getLocalTime() — reads bot.time
//   - temperature String → bot.weather.temp
//   - DynamicJsonDocument 4096 → 1024
//   - Location reads from bot.weather.latitude
//     and bot.weather.longitude (set via BLE)
//   - Default coordinates Kollam until BLE sends
//   - Added Display_init() wrapper
//   - Added Display_render() wrapper
//   - Added Weather_fetch() wrapper
//   - Connected to bot_state.h
// ============================================

#include "display_manager.h"
#include "animation_engine.h"
#include "bot_state.h"
#include "pins.h"
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ── Shared display object ─────────────────────
// Defined HERE — animation_engine uses extern
//U8G2_SH1106_128X64_NONAME_F_HW_I2C display(
  //U8G2_R0,
  //U8X8_PIN_NONE,
  //PIN_OLED_SCL,
  //PIN_OLED_SDA
//);
U8G2_SH1106_128X64_NONAME_F_HW_I2C display(
  U8G2_R0,
  U8X8_PIN_NONE,
  PIN_OLED_SCL,
  PIN_OLED_SDA
);

// ── Eye movement variables ────────────────────
static int           _eyeOffset   = 0;
static bool          _moveRight   = true;
static unsigned long _lastEyeMove = 0;

// ── Blink variables ───────────────────────────
static bool          _blinking  = false;
static unsigned long _lastBlink = 0;

// ── Display_init ─────────────────────────────
void Display_init() {
  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
  display.begin();
  display.setContrast(255);

  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(20, 25, "INNO");
  display.drawStr(10, 40, "TableBot");
  display.sendBuffer();
  delay(1000);

  // Set default location — Kollam, Kerala
  // Will be overwritten when Niranjan's app
  // sends "LAT:x.xxxx,LON:x.xxxx" via BLE
  bot.weather.latitude  = 8.8932;
  bot.weather.longitude = 76.6141;

  Serial.println("Display: Ready");
  Serial.println("Location: Kollam default — waiting for BLE update");
}

// ── Internal: getWeather ──────────────────────
// Uses bot.weather.latitude and longitude
// These are set by BLE command from Niranjan's app
// Until app sends location — Kollam is used
static bool _getWeather(BotState& bot) {
  if (WiFi.status() != WL_CONNECTED)
    return false;

  HTTPClient http;

  // Location comes from bot struct
  // Set by BLE in mode_manager.cpp
  // Default is Kollam until phone updates it
  String url =
    "https://api.open-meteo.com/v1/forecast?latitude=" +
    String(bot.weather.latitude, 4) +
    "&longitude=" +
    String(bot.weather.longitude, 4) +
    "&current=temperature_2m";

  http.begin(url);
  int code = http.GET();

  if (code == 200) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, http.getString());

    bot.weather.temp    =
      doc["current"]["temperature_2m"];
    bot.weather.isValid = true;

    Serial.print("Weather: ");
    Serial.print(bot.weather.temp);
    Serial.print("C at LAT:");
    Serial.print(bot.weather.latitude);
    Serial.print(" LON:");
    Serial.println(bot.weather.longitude);

    http.end();
    return true;
  }

  http.end();
  bot.weather.isValid = false;
  Serial.println("Weather: Fetch failed");
  return false;
}

// ── Internal: drawHomepage ────────────────────
static void _drawHomepage(BotState& bot) {

  if (millis() - _lastEyeMove > 2500) {
    _lastEyeMove = millis();
    _eyeOffset   = _moveRight ? 4 : -4;
    _moveRight   = !_moveRight;
  }

  if (millis() - _lastBlink > 5000) {
    _blinking  = true;
    _lastBlink = millis();
  }

  // Read time from bot struct
  // main.ino updates this every second
  char timeString[10];
  snprintf(timeString, sizeof(timeString),
           "%02d:%02d",
           bot.time.hour,
           bot.time.minute);

  // Read weather from bot struct
  char weatherText[12];
  if (bot.weather.isValid) {
    snprintf(weatherText, sizeof(weatherText),
             "%dC", (int)bot.weather.temp);
  } else {
    snprintf(weatherText, sizeof(weatherText), "--C");
  }

  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);

  // Time — top left
  display.drawStr(0, 10, timeString);

  // Weather — top right
  display.drawStr(94, 10, weatherText);

  // Face outline
  display.drawRFrame(18, 12, 92, 48, 8);

  // Eyes
  if (!_blinking) {
    display.drawDisc(45, 32, 9);
    display.drawDisc(83, 32, 9);
    display.setDrawColor(0);
    display.drawDisc(45 + _eyeOffset, 32, 4);
    display.drawDisc(83 + _eyeOffset, 32, 4);
    display.setDrawColor(1);
    display.drawDisc(43 + _eyeOffset, 30, 1);
    display.drawDisc(81 + _eyeOffset, 30, 1);
  } else {
    display.drawLine(36, 32, 54, 32);
    display.drawLine(74, 32, 92, 32);
    if (millis() - _lastBlink > 120) {
      _blinking = false;
    }
  }

  // Cheeks
  display.drawCircle(30, 44, 3);
  display.drawCircle(98, 44, 3);

  // Smile
  display.drawLine(56, 47, 64, 51);
  display.drawLine(64, 51, 72, 47);

  // BLE icon
  if (bot.display.showBLEIcon) {
    display.drawDisc(122, 5, 3);
  }

  // Battery low warning
  if (bot.display.showBatteryIcon) {
    display.setFont(u8g2_font_5x7_tf);
    display.drawStr(0, 64, "!");
  }

  display.sendBuffer();
}

// ── Internal: drawFocusMode ──────────────────
static void _drawFocusMode(BotState& bot) {
  char timeString[10];
  snprintf(timeString, sizeof(timeString),
           "%02d:%02d",
           bot.time.hour,
           bot.time.minute);

  display.clearBuffer();

  // Big clock centered
  display.setFont(u8g2_font_logisoso24_tf);
  int tw = display.getStrWidth(timeString);
  display.drawStr((128 - tw) / 2, 42, timeString);

  // Small calm eyes above clock
  display.setDrawColor(1);
  display.drawDisc(44, 14, 6);
  display.drawDisc(84, 14, 6);
  display.setDrawColor(0);
  display.drawDisc(44, 14, 3);
  display.drawDisc(84, 14, 3);
  display.setDrawColor(1);

  // BLE icon
  if (bot.display.showBLEIcon) {
    display.drawDisc(122, 5, 3);
  }

  display.sendBuffer();
}

// ── Internal: drawSleeping ───────────────────
static void _drawSleeping() {
  display.clearBuffer();
  display.sendBuffer();
}

// ── Internal: drawError ──────────────────────
static void _drawError() {
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(25, 25, "No WiFi");
  display.drawStr(20, 40, "Tap to retry");
  display.sendBuffer();
}

// ── Internal: drawUpdating ───────────────────
static void _drawUpdating() {
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(15, 35, "Updating...");
  display.sendBuffer();
}

// ══════════════════════════════════════════════
// Display_render — MAIN WRAPPER
// Called every 100ms from main.ino
// ══════════════════════════════════════════════
void Display_render(BotState& bot) {

  if (bot.mode == MODE_GAME)         return;
  if (bot.state == STATE_SLEEPING) { _drawSleeping();  return; }
  if (bot.state == STATE_ERROR)    { _drawError();     return; }
  if (bot.state == STATE_UPDATING) { _drawUpdating();  return; }
  if (bot.mode  == MODE_FOCUS)     { _drawFocusMode(bot); return; }

  _drawHomepage(bot);
}

// ══════════════════════════════════════════════
// Weather_fetch — WRAPPER
// Called every 10 min from main.ino
// ══════════════════════════════════════════════
bool Weather_fetch(BotState& bot) {
  return _getWeather(bot);
}