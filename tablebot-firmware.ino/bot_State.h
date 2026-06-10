// ============================================
// bot_state.h — INNO TableBot Stage 2 FINAL
// Last updated: June 2026
// Board: Waveshare ESP32-S3-Tiny
// ============================================

#ifndef BOT_STATE_H
#define BOT_STATE_H

#include <Arduino.h>

// ── SYSTEM STATES ────────────────────────────
typedef enum {
  STATE_ACTIVE,      // normal — face, clock, weather
  STATE_SLEEPING,    // deep sleep — RTC keeps time
  STATE_UPDATING,    // fetching weather
  STATE_ERROR,       // something failed
  STATE_LISTENING,   // AI — recording voice
  STATE_THINKING,    // AI — waiting for response
  STATE_SPEAKING     // AI — playing response
} SystemState;

// ── MODES ────────────────────────────────────
typedef enum {
  MODE_ACTIVE,       // normal face, clock, weather
  MODE_FOCUS,        // big clock, calm face, muted
  MODE_GAME,         // full screen game
  MODE_AI            // voice conversation
} BotMode;

// ── FACE EXPRESSIONS ─────────────────────────
typedef enum {
  FACE_HAPPY,        // active mode default
  FACE_FOCUSED,      // focus mode — calm eyes
  FACE_LISTENING,    // AI — wide alert eyes
  FACE_THINKING,     // AI — eyes looking up
  FACE_SPEAKING,     // AI — eyes + mouth animating
  FACE_ALARM,        // alarm ringing
  FACE_SAD           // error state only
} FaceExpression;

// ── TIME DATA ────────────────────────────────
// Updated every second from built-in ESP32 RTC
// NTP syncs on boot and every hour
typedef struct {
  int  hour;
  int  minute;
  int  second;
  int  day;
  int  month;
  int  year;
  char dayName[10];
} TimeData;

// ── WEATHER DATA ─────────────────────────────
// Open-Meteo API — no API key needed
// Location updated via BLE from app
typedef struct {
  float temp;
  float humidity;
  char  description[32];
  bool  isValid;
  float latitude;
  float longitude;
} WeatherData;

// ── FACE / ANIMATION DATA ────────────────────
typedef struct {
  FaceExpression currentExpression;
  FaceExpression targetExpression;
  int            frame;
  bool           isBlinking;
  bool           isAnimating;
  bool           showMouth;      // ONLY true in SPEAKING
  unsigned long  lastBlinkMs;
  unsigned long  lastMorphMs;
} FaceState;

// ── DISPLAY CONFIG ───────────────────────────
// Pranav reads this in display_manager.cpp
typedef struct {
  bool showWeather;
  bool showFace;
  bool showClock;
  bool bigClock;
  bool showBLEIcon;
  bool showBatteryIcon;
  bool showGame;
  bool showAIResponse;
  bool showAlarm;
  bool showReminder;
} DisplayConfig;

// ── AUDIO DATA ───────────────────────────────
typedef struct {
  bool isMuted;
  bool isPlaying;
  bool isRecording;
  int  volume;        // 0-100
} AudioState;

// ── BLE DATA ─────────────────────────────────
typedef struct {
  bool isConnected;
  bool newCommandReceived;
  char lastCommand[128];  // increased for API key
} BLEState;

// ── BATTERY DATA ─────────────────────────────
typedef struct {
  int  percentage;    // 0-100
  bool isCharging;
  bool isLow;         // true below 20%
} BatteryState;

// ── RGB LED DATA ─────────────────────────────
// External RGB LED — battery indicator
// Onboard WS2812 — BLE indicator
typedef struct {
  bool  bleConnected;    // WS2812 blue on/off
  uint8_t rgbR;          // RGB LED red value
  uint8_t rgbG;          // RGB LED green value
  uint8_t rgbB;          // RGB LED blue value
} LEDState;

// ── GAME DATA ────────────────────────────────
typedef struct {
  bool isRunning;
  int  score;
  int  highScore;
  bool isDead;
  bool flapPressed;
  int  selectedGame;    // 0=FlappyBird, 1=Snake etc
} GameState;

// ── AI DATA ──────────────────────────────────
typedef struct {
  bool  isActive;
  bool  isRecording;
  bool  isWaitingResponse;
  char  userText[256];
  char  responseText[512];
  bool  hasResponse;
  char  apiKey[64];         // received via BLE
  unsigned long recordStart;
} AIData;

// ── ALARM DATA ───────────────────────────────
typedef struct {
  bool isSet;
  int  hour;
  int  minute;
  char label[32];
  bool isRinging;
} AlarmData;

// ── REMINDER DATA ────────────────────────────
typedef struct {
  bool isSet;
  int  hour;
  int  minute;
  char text[64];
  bool isShowing;
} ReminderData;

// ══════════════════════════════════════════════
// MAIN STRUCT
// ══════════════════════════════════════════════
typedef struct {
  SystemState   state;
  BotMode       mode;

  TimeData      time;
  WeatherData   weather;
  FaceState     face;
  DisplayConfig display;
  AudioState    audio;
  BLEState      ble;
  BatteryState  battery;
  LEDState      led;
  GameState     game;
  AIData        ai;
  AlarmData     alarm;
  ReminderData  reminders[5];

  // ── Timing trackers ──────────────────────
  unsigned long lastTouchMs;
  unsigned long lastWeatherFetchMs;
  unsigned long lastNTPSyncMs;
  unsigned long lastBLECheckMs;
  unsigned long lastAnimFrameMs;
  unsigned long lastClockUpdateMs;
  unsigned long lastIdleCheckMs;
  unsigned long lastBatteryCheckMs;
  unsigned long lastRandomLookMs;

  // ── System flags ─────────────────────────
  bool wifiConnected;
  bool ntpSynced;
  bool firstBootDone;

} BotState;

// ── GLOBAL INSTANCE ──────────────────────────
extern BotState bot;

// ── BLE COMMAND REFERENCE ────────────────────
// MODE:ACTIVE        → active mode
// MODE:FOCUS         → focus mode
// MODE:GAME          → game mode
// MODE:AI            → AI mode
// APIKEY:xxxxxxxx    → set Gemini API key
// ALARM:HH:MM:LABEL  → set alarm
// REMIND:HH:MM:TEXT  → set reminder
// LAT:x.xxxx,LON:x   → update location
// VOL:80             → set volume 0-100

// ── MOUTH RULE ───────────────────────────────
// bot.face.showMouth = true ONLY in STATE_SPEAKING
// All other states → showMouth = false
// Pranav draws mouth ONLY when showMouth is true

// ── RGB LED COLOR REFERENCE ──────────────────
// Battery > 60%  → Green  (R=0,   G=255, B=0)
// Battery 20-60% → Orange (R=255, G=165, B=0)
// Battery < 20%  → Red    (R=255, G=0,   B=0)
// Charging       → Red pulsing

#endif