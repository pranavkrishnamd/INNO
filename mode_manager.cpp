// ============================================
// mode_manager.cpp — INNO TableBot Stage 2
// BLE command unification + mode switching
// Last updated: June 2026
// ============================================

#include "mode_manager.h"
#include "state_manager.h"
#include <Arduino.h>
#include <string.h>
#include <stdlib.h>

// ── Forward declarations ──────────────────────
static void applyDisplayConfig(BotState &bot);
static void parseBLECommand(BotState &bot, const char* cmd);

// ════════════════════════════════════════════
// Mode_init
// Called once in setup() after State_init
// ════════════════════════════════════════════
void Mode_init(BotState &bot) {
  Mode_setActive(bot);
  Serial.println("Mode: Initialized → ACTIVE");
}

// ════════════════════════════════════════════
// Mode_apply
// Called from loop() when newCommandReceived
// Reads bot.ble.lastCommand and dispatches
// ════════════════════════════════════════════
void Mode_apply(BotState &bot) {
  if (!bot.ble.newCommandReceived) return;
  bot.ble.newCommandReceived = false;

  parseBLECommand(bot, bot.ble.lastCommand);
}

// ════════════════════════════════════════════
// parseBLECommand
// Unified BLE command parser
// All commands from Niranjan's app go here
// ════════════════════════════════════════════
static void parseBLECommand(BotState &bot, const char* cmd) {
  Serial.print("BLE CMD: ");
  Serial.println(cmd);

  // ── MODE commands ────────────────────────
  if (strcmp(cmd, "MODE:ACTIVE") == 0) {
    Mode_setActive(bot);
    return;
  }
  if (strcmp(cmd, "MODE:FOCUS") == 0) {
    Mode_setFocus(bot);
    return;
  }
  if (strcmp(cmd, "MODE:GAME") == 0) {
    Mode_setGame(bot);
    return;
  }
  if (strcmp(cmd, "MODE:AI") == 0) {
    Mode_setAI(bot);
    return;
  }

  // ── APIKEY:xxxxxxxx ──────────────────────
  // Gemini API key delivered from Niranjan's app
  if (strncmp(cmd, "APIKEY:", 7) == 0) {
    Mode_handleAPIKey(bot, cmd + 7);
    return;
  }

  // ── VOL:0-100 ────────────────────────────
  if (strncmp(cmd, "VOL:", 4) == 0) {
    int vol = atoi(cmd + 4);
    vol = constrain(vol, 0, 100);
    Mode_handleVolume(bot, vol);
    return;
  }

  // ── LAT:x.xxxx,LON:x.xxxx ───────────────
  if (strncmp(cmd, "LAT:", 4) == 0) {
    // Expected format: LAT:8.8932,LON:76.6141
    const char* latStr = cmd + 4;
    const char* lonPtr = strstr(cmd, ",LON:");
    if (lonPtr != NULL) {
      float lat = atof(latStr);
      float lon = atof(lonPtr + 5);
      Mode_handleLocation(bot, lat, lon);
    } else {
      Serial.println("Mode: LAT/LON parse error");
    }
    return;
  }

  // ── ALARM:HH:MM:LABEL ───────────────────
  if (strncmp(cmd, "ALARM:", 6) == 0) {
    // Format: ALARM:07:30:Wake Up
    char buf[128];
    strncpy(buf, cmd + 6, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char* p1 = strchr(buf, ':');
    if (p1 == NULL) { Serial.println("Mode: ALARM parse error"); return; }
    *p1 = '\0';
    int hour = atoi(buf);

    char* p2 = strchr(p1 + 1, ':');
    if (p2 == NULL) { Serial.println("Mode: ALARM parse error"); return; }
    *p2 = '\0';
    int minute = atoi(p1 + 1);

    const char* label = p2 + 1;
    Mode_handleAlarm(bot, hour, minute, label);
    return;
  }

  // ── REMIND:HH:MM:TEXT ───────────────────
  if (strncmp(cmd, "REMIND:", 7) == 0) {
    // Format: REMIND:09:00:Take medicine
    char buf[128];
    strncpy(buf, cmd + 7, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char* p1 = strchr(buf, ':');
    if (p1 == NULL) { Serial.println("Mode: REMIND parse error"); return; }
    *p1 = '\0';
    int hour = atoi(buf);

    char* p2 = strchr(p1 + 1, ':');
    if (p2 == NULL) { Serial.println("Mode: REMIND parse error"); return; }
    *p2 = '\0';
    int minute = atoi(p1 + 1);

    const char* text = p2 + 1;
    Mode_handleReminder(bot, hour, minute, text);
    return;
  }

  Serial.print("Mode: Unknown command → ");
  Serial.println(cmd);
}

// ════════════════════════════════════════════
// MODE SETTERS
// Each one updates state, face, display config
// ════════════════════════════════════════════

void Mode_setActive(BotState &bot) {
  bot.mode                      = MODE_ACTIVE;
  bot.state                     = STATE_ACTIVE;
  bot.face.currentExpression    = FACE_HAPPY;
  bot.face.targetExpression     = FACE_HAPPY;
  bot.face.showMouth            = false;
  bot.audio.isMuted             = false;

  bot.display.showFace          = true;
  bot.display.showClock         = true;
  bot.display.showWeather       = true;
  bot.display.bigClock          = false;
  bot.display.showGame          = false;
  bot.display.showAIResponse    = false;

  applyDisplayConfig(bot);
  Serial.println("Mode: → ACTIVE");
}

void Mode_setFocus(BotState &bot) {
  bot.mode                      = MODE_FOCUS;
  bot.state                     = STATE_ACTIVE;
  bot.face.currentExpression    = FACE_FOCUSED;
  bot.face.targetExpression     = FACE_FOCUSED;
  bot.face.showMouth            = false;
  bot.audio.isMuted             = true;   // muted in focus

  bot.display.showFace          = true;
  bot.display.showClock         = true;
  bot.display.showWeather       = false;
  bot.display.bigClock          = true;   // big clock
  bot.display.showGame          = false;
  bot.display.showAIResponse    = false;

  applyDisplayConfig(bot);
  Serial.println("Mode: → FOCUS");
}

void Mode_setGame(BotState &bot) {
  bot.mode                      = MODE_GAME;
  bot.state                     = STATE_ACTIVE;
  bot.face.showMouth            = false;

  bot.display.showFace          = false;
  bot.display.showClock         = false;
  bot.display.showWeather       = false;
  bot.display.bigClock          = false;
  bot.display.showGame          = true;
  bot.display.showAIResponse    = false;

  bot.game.isRunning            = true;
  bot.game.isDead               = false;
  bot.game.score                = 0;

  applyDisplayConfig(bot);
  Serial.println("Mode: → GAME");
}

void Mode_setAI(BotState &bot) {
  // Guard — no API key yet
  if (strlen(bot.ai.apiKey) == 0) {
    Serial.println("Mode: AI blocked — no API key");
    BLE_sendMessage("STATE:NO_APIKEY");
    return;
  }

  bot.mode                      = MODE_AI;
  bot.state                     = STATE_ACTIVE;  // idle until touch
  bot.face.currentExpression    = FACE_HAPPY;
  bot.face.targetExpression     = FACE_HAPPY;
  bot.face.showMouth            = false;

  bot.display.showFace          = true;
  bot.display.showClock         = false;
  bot.display.showWeather       = false;
  bot.display.bigClock          = false;
  bot.display.showGame          = false;
  bot.display.showAIResponse    = false;

  bot.ai.isActive               = true;
  bot.ai.isRecording            = false;
  bot.ai.isWaitingResponse      = false;
  bot.ai.hasResponse            = false;

  applyDisplayConfig(bot);
  Serial.println("Mode: → AI");
}

void Mode_exitGame(BotState &bot) {
  bot.game.isRunning = false;
  bot.game.isDead    = false;
  Mode_setActive(bot);
  Serial.println("Mode: Game exited → ACTIVE");
}

// ════════════════════════════════════════════
// BLE COMMAND HANDLERS
// ════════════════════════════════════════════

void Mode_handleAPIKey(BotState &bot, const char* key) {
  if (strlen(key) == 0) {
    Serial.println("Mode: APIKEY empty — ignored");
    return;
  }
  strncpy(bot.ai.apiKey, key, sizeof(bot.ai.apiKey) - 1);
  bot.ai.apiKey[sizeof(bot.ai.apiKey) - 1] = '\0';
  Serial.println("Mode: API key stored ✓");
  BLE_sendMessage("STATE:APIKEY_OK");
}

void Mode_handleVolume(BotState &bot, int vol) {
  bot.audio.volume = vol;
  bot.audio.isMuted = (vol == 0);
  Serial.print("Mode: Volume → ");
  Serial.println(vol);
  // audio_module will read bot.audio.volume for I2S gain
}

void Mode_handleLocation(BotState &bot, float lat, float lon) {
  bot.weather.latitude  = lat;
  bot.weather.longitude = lon;
  Serial.print("Mode: Location → ");
  Serial.print(lat, 4);
  Serial.print(", ");
  Serial.println(lon, 4);
  // weather fetch will use new coords on next interval
}

void Mode_handleAlarm(BotState &bot, int hour, int minute, const char* label) {
  bot.alarm.isSet    = true;
  bot.alarm.hour     = hour;
  bot.alarm.minute   = minute;
  bot.alarm.isRinging = false;
  strncpy(bot.alarm.label, label, sizeof(bot.alarm.label) - 1);
  bot.alarm.label[sizeof(bot.alarm.label) - 1] = '\0';

  Serial.print("Mode: Alarm set → ");
  Serial.print(hour);
  Serial.print(":");
  Serial.print(minute < 10 ? "0" : "");
  Serial.print(minute);
  Serial.print(" — ");
  Serial.println(label);
}

void Mode_handleReminder(BotState &bot, int hour, int minute, const char* text) {
  // Find first free slot in reminders[5]
  for (int i = 0; i < 5; i++) {
    if (!bot.reminders[i].isSet) {
      bot.reminders[i].isSet    = true;
      bot.reminders[i].hour     = hour;
      bot.reminders[i].minute   = minute;
      bot.reminders[i].isShowing = false;
      strncpy(bot.reminders[i].text, text,
              sizeof(bot.reminders[i].text) - 1);
      bot.reminders[i].text[sizeof(bot.reminders[i].text) - 1] = '\0';

      Serial.print("Mode: Reminder[");
      Serial.print(i);
      Serial.print("] → ");
      Serial.print(hour);
      Serial.print(":");
      Serial.print(minute < 10 ? "0" : "");
      Serial.print(minute);
      Serial.print(" — ");
      Serial.println(text);
      return;
    }
  }
  Serial.println("Mode: Reminder slots full — ignored");
}

// ════════════════════════════════════════════
// applyDisplayConfig
// Called after every mode switch
// Prints current display flags to Serial
// ════════════════════════════════════════════
static void applyDisplayConfig(BotState &bot) {
  Serial.print("Display: face=");
  Serial.print(bot.display.showFace);
  Serial.print(" clock=");
  Serial.print(bot.display.showClock);
  Serial.print(" bigClock=");
  Serial.print(bot.display.bigClock);
  Serial.print(" weather=");
  Serial.print(bot.display.showWeather);
  Serial.print(" game=");
  Serial.println(bot.display.showGame);
}