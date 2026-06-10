// ============================================
// mode_manager.cpp — TableBot
// Updated: added game mode, servo handling
// ============================================

#include "mode_manager.h"
#include "bot_state.h"
#include "pins.h"
#include <string.h>

void Mode_init(BotState& bot) {
  Mode_setIdle(bot);
  Serial.println("Mode Manager: Initialized → IDLE");
}

void Mode_apply(BotState& bot) {
  // Clear flag first — don't apply twice
  bot.ble.newCommandReceived = false;

  if (strcmp(bot.ble.lastCommand, "MODE:IDLE") == 0) {
    Mode_setIdle(bot);
  }
  else if (strcmp(bot.ble.lastCommand, "MODE:FOCUS") == 0) {
    Mode_setFocus(bot);
  }
  else if (strcmp(bot.ble.lastCommand, "MODE:GAME") == 0) {
    Mode_setGame(bot);
  }
  else {
    Serial.print("Mode Manager: Unknown command → ");
    Serial.println(bot.ble.lastCommand);
  }
}

void Mode_setIdle(BotState& bot) {
  bot.mode = MODE_IDLE;

  // Face — animated
  bot.face.targetExpression  = FACE_HAPPY;
  bot.face.isAnimating       = true;
  bot.face.isBlinking        = true;

  // Display — everything visible
  bot.display.showFace       = true;
  bot.display.showWeather    = true;
  bot.display.showClock      = true;
  bot.display.bigClock       = false;
  bot.display.showGame       = false;

  // Audio — unmute
  bot.audio.isMuted          = false;
  digitalWrite(PIN_AMP_SD_MODE, HIGH);

  // Game — not running
  bot.game.isRunning         = false;
  bot.game.flapPressed       = false;

  Serial.println("Mode Manager: → IDLE MODE");
}

void Mode_setFocus(BotState& bot) {
  bot.mode = MODE_FOCUS;

  // Face — static focused
  bot.face.targetExpression  = FACE_FOCUSED;
  bot.face.currentExpression = FACE_FOCUSED;
  bot.face.isAnimating       = false;
  bot.face.isBlinking        = false;

  // Display — clock only
  bot.display.showFace       = true;
  bot.display.showWeather    = false;
  bot.display.showClock      = true;
  bot.display.bigClock       = true;
  bot.display.showGame       = false;

  // Audio — mute
  bot.audio.isMuted          = true;
  digitalWrite(PIN_AMP_SD_MODE, LOW);

  // Game — not running
  bot.game.isRunning         = false;
  bot.game.flapPressed       = false;

  Serial.println("Mode Manager: → FOCUS MODE");
}

void Mode_setGame(BotState& bot) {
  bot.mode = MODE_GAME;

  // Face — hide during game
  bot.face.isAnimating       = false;
  bot.face.isBlinking        = false;

  // Display — game only, nothing else
  bot.display.showFace       = false;
  bot.display.showWeather    = false;
  bot.display.showClock      = false;
  bot.display.bigClock       = false;
  bot.display.showGame       = true;

  // Audio — game decides its own sounds
  bot.audio.isMuted          = false;
  digitalWrite(PIN_AMP_SD_MODE, HIGH);
  
  // Game — start fresh
  bot.game.isRunning         = true;
  bot.game.score             = 0;
  bot.game.isDead            = false;
  bot.game.flapPressed       = false;

  // Reset touch timer so bot doesn't
  // immediately sleep when game ends
  bot.lastTouchMs            = millis();

  Serial.println("Mode Manager: → GAME MODE");
}

void Mode_exitGame(BotState& bot) {
  // Save high score before exiting
  if (bot.game.score > bot.game.highScore) {
    bot.game.highScore = bot.game.score;
  }
  // Go back to idle
  Mode_setIdle(bot);
  Serial.print("Mode Manager: GAME → IDLE | Score: ");
  Serial.print(bot.game.score);
  Serial.print(" | Best: ");
  Serial.println(bot.game.highScore);
}