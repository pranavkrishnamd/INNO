// ============================================
// state_manager.cpp — TableBot
// Watches what's happening and switches states
// Updated: game mode touch handling + servo
// ============================================

#include "state_manager.h"
#include "bot_state.h"

#define IDLE_TIMEOUT_MS  30000  // 30 seconds to sleep

void State_init(BotState& bot) {
  bot.state                  = STATE_IDLE;
  bot.mode                   = MODE_IDLE;
  bot.lastTouchMs            = millis();

  // Face
  bot.face.currentExpression = FACE_HAPPY;
  bot.face.targetExpression  = FACE_HAPPY;
  bot.face.isBlinking        = true;
  bot.face.isAnimating       = true;

  // Display — show everything in idle
  bot.display.showWeather    = true;
  bot.display.showFace       = true;
  bot.display.showClock      = true;
  bot.display.bigClock       = false;
  bot.display.showBLEIcon    = false;
  bot.display.showBatteryIcon= false;
  bot.display.showGame       = false;

  // Audio
  bot.audio.isMuted          = false;
  bot.audio.volume           = 80;

  // Game
  bot.game.isRunning         = false;
  bot.game.score             = 0;
  bot.game.highScore         = 0;
  bot.game.isDead            = false;
  bot.game.flapPressed       = false;

  Serial.println("State Manager: Initialized → IDLE");
}

void State_onTouch(BotState& bot) {
  // During game — Sensor 1 = FLAP only
  // Game manager handles this directly
  // State manager does nothing during game
  if (bot.mode == MODE_GAME) {
    bot.game.flapPressed = true;
    Serial.println("State Manager: Flap!");
    return;
  }

  // Normal touch behavior outside game
  bot.lastTouchMs = millis();

  if (bot.state == STATE_SLEEPING) {
    bot.state                  = STATE_IDLE;
    bot.face.isAnimating       = true;
    bot.face.isBlinking        = true;
    bot.face.targetExpression  = FACE_HAPPY;
    bot.display.showWeather    = true;
    bot.display.showFace       = true;
    bot.display.showClock      = true;
    bot.display.bigClock       = false;
  }

  else if (bot.state == STATE_IDLE) {
    bot.state                  = STATE_ACTIVE;
    bot.face.targetExpression = FACE_HAPPY;
    Serial.println("State Manager: IDLE → ACTIVE");
  }

  else if (bot.state == STATE_ACTIVE) {
    bot.state                  = STATE_IDLE;
    bot.face.targetExpression  = FACE_HAPPY;
    Serial.println("State Manager: ACTIVE → IDLE");
  }

  else if (bot.state == STATE_ERROR) {
    State_onRecovery(bot);
  }
}

void State_checkIdle(BotState& bot) {
  // Don't sleep during game
  if (bot.mode == MODE_GAME) return;
  if (bot.state != STATE_IDLE) return;

  unsigned long timeSinceTouch =
    millis() - bot.lastTouchMs;

  if (timeSinceTouch >= IDLE_TIMEOUT_MS) {
    bot.state                  = STATE_SLEEPING;
    bot.face.isAnimating       = false;
    bot.face.isBlinking        = false;
    bot.face.currentExpression = FACE_SLEEPY;
    bot.display.showWeather    = false;
    bot.display.showFace       = false;
    bot.display.showClock      = false;
    Serial.println("State Manager: IDLE → SLEEPING");
  }
}

void State_onError(BotState& bot) {
  bot.state                  = STATE_ERROR;
  bot.face.targetExpression  = FACE_SAD;
  bot.face.isAnimating       = false;
  bot.face.isBlinking        = false;
  bot.display.showWeather    = false;
  Serial.println("State Manager: → ERROR");
}

void State_onRecovery(BotState& bot) {
  bot.state                  = STATE_IDLE;
  bot.face.targetExpression  = FACE_HAPPY;
  bot.face.isAnimating       = true;
  bot.face.isBlinking        = true;
  bot.display.showWeather    = true;
  bot.display.showFace       = true;
  bot.display.showClock      = true;
  bot.lastTouchMs            = millis();
  Serial.println("State Manager: ERROR → IDLE");
}

void State_onUpdateStart(BotState& bot) {
  bot.state                  = STATE_UPDATING;
  bot.face.targetExpression  = FACE_NEUTRAL;
  Serial.println("State Manager: → UPDATING");
}

void State_onUpdateDone(BotState& bot) {
  bot.state                  = STATE_IDLE;
  bot.face.targetExpression  = FACE_HAPPY;
  Serial.println("State Manager: UPDATING → IDLE");
}