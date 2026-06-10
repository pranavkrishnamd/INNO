// ============================================
// game_engine.cpp — TableBot
// Flappy Bird game engine
// Original game code by Pranav
// Integrated by Ribin — May 2026
// Changes from original:
//   - Removed setup() and loop()
//   - Removed own display object — uses shared
//   - Removed #define TOUCH_PIN — uses pins.h
//   - Removed while(true) game loop
//   - Removed all delay() calls
//   - Replaced with Game_tick() — non-blocking
//   - score and highScore → bot.game.score
//     and bot.game.highScore
//   - Touch input → reads bot.game.flapPressed
//     set by state_manager when Sensor 1 tapped
//   - Game over → sets bot.game.isDead = true
//     Sensor 1 restarts, Sensor 2 exits
//   - All physics constants unchanged
// ============================================

#include "game_engine.h"
#include "bot_state.h"
#include "pins.h"
#include <Arduino.h>

// ── Physics constants — unchanged from original
static const float GRAVITY    =  0.25;
static const float FLAP       = -2.8;
static const int   BIRD_X     =  30;
static const int   BIRD_R     =  3;
static const int   PIPE_W     =  14;
static const int   PIPE_GAP   =  26;
static const int   PIPE_SPEED =  2;

// ── Game state variables ──────────────────────
static float birdY    = 32;
static float birdVY   = 0;
static int   pipeX    = 128;
static int   pipeGapY = 20;
static bool  scored   = false;

// ── Game phases ───────────────────────────────
// Tracks what screen to show
typedef enum {
  GAME_WAITING,    // waiting for first tap to start
  GAME_PLAYING,    // game actively running
  GAME_OVER        // game over screen showing
} GamePhase;

static GamePhase     gamePhase        = GAME_WAITING;
static unsigned long gameOverShownAt  = 0;

// ══════════════════════════════════════════════
// INTERNAL HELPERS — drawing functions
// All unchanged from original except
// u8g2 → display (shared object)
// ══════════════════════════════════════════════

// ── Draw pipes and bird and score ────────────
static void _drawGame(BotState& bot) {
  display.clearBuffer();

  // Top pipe
  display.drawBox(pipeX, 0, PIPE_W, pipeGapY);

  // Bottom pipe
  display.drawBox(
    pipeX,
    pipeGapY + PIPE_GAP,
    PIPE_W,
    64 - (pipeGapY + PIPE_GAP)
  );

  // Bird body
  display.drawDisc(BIRD_X, (int)birdY, BIRD_R);

  // Bird eye
  display.setDrawColor(0);
  display.drawDisc(BIRD_X + 1, (int)birdY - 1, 1);
  display.setDrawColor(1);

  // Score — top left
  char scoreText[10];
  sprintf(scoreText, "%d", bot.game.score);
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(5, 10, scoreText);

  display.sendBuffer();
}

// ── Draw waiting screen ───────────────────────
static void _drawWaiting() {
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(20, 25, "FLAPPY BIRD");
  display.drawStr(10, 45, "TAP TO START");
  display.sendBuffer();
}

// ── Draw game over screen ─────────────────────
static void _drawGameOver(BotState& bot) {
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(30, 15, "GAME OVER");

  char scoreText[20];
  sprintf(scoreText, "SCORE: %d", bot.game.score);
  display.drawStr(25, 32, scoreText);

  sprintf(scoreText, "BEST:  %d", bot.game.highScore);
  display.drawStr(25, 46, scoreText);

  display.drawStr(8, 60, "TAP=RETRY  HOLD=EXIT");

  display.sendBuffer();
}

// ── Collision detection — unchanged from original
static bool _collide() {
  // Hit top or bottom of screen
  if (birdY < 0 || birdY > 64)
    return true;

  // Hit pipe
  if (BIRD_X + BIRD_R > pipeX &&
      BIRD_X - BIRD_R < pipeX + PIPE_W) {
    if (birdY < pipeGapY ||
        birdY > pipeGapY + PIPE_GAP)
      return true;
  }
  return false;
}

// ── Reset all game variables ──────────────────
static void _resetGame(BotState& bot) {
  birdY              = 32;
  birdVY             = 0;
  pipeX              = 128;
  pipeGapY           = random(8, 38);
  scored             = false;
  bot.game.score     = 0;
  bot.game.isDead    = false;
  bot.game.flapPressed = false;
  gamePhase          = GAME_PLAYING;
}

// ══════════════════════════════════════════════
// Game_init — call once in setup()
// ══════════════════════════════════════════════
void Game_init() {
  gamePhase = GAME_WAITING;
  Serial.println("Game Engine: Ready");
}

// ══════════════════════════════════════════════
// Game_reset — call when entering game
// Called from Mode_setGame() via main.ino
// ══════════════════════════════════════════════
void Game_reset(BotState& bot) {
  birdY              = 32;
  birdVY             = 0;
  pipeX              = 128;
  pipeGapY           = random(8, 38);
  scored             = false;
  bot.game.score     = 0;
  bot.game.isDead    = false;
  bot.game.flapPressed = false;
  gamePhase          = GAME_WAITING;
  _drawWaiting();
  Serial.println("Game Engine: Reset — waiting for tap");
}

// ══════════════════════════════════════════════
// Game_tick — MAIN FUNCTION
// Called every 20ms from main.ino
// Non-blocking — no delay() anywhere
// ══════════════════════════════════════════════
void Game_tick(BotState& bot) {

  // Only run if game mode is active
  if (!bot.game.isRunning) return;

  // ── WAITING phase ─────────────────────────
  // Show start screen until first tap
  if (gamePhase == GAME_WAITING) {
    _drawWaiting();
    // First tap starts the game
    if (bot.game.flapPressed) {
      bot.game.flapPressed = false;
      _resetGame(bot);
      Serial.println("Game Engine: Started!");
    }
    return;
  }

  // ── GAME OVER phase ───────────────────────
  // Show game over screen for 2.5 seconds
  // Sensor 1 tap = restart
  // Sensor 2 tap = exit (handled in main.ino)
  if (gamePhase == GAME_OVER) {
    _drawGameOver(bot);

    // Sensor 1 tap — restart game
    if (bot.game.flapPressed) {
      bot.game.flapPressed = false;

      // Save high score before reset
      if (bot.game.score > bot.game.highScore) {
        bot.game.highScore = bot.game.score;
      }

      _resetGame(bot);
      Serial.println("Game Engine: Restarted");
    }
    return;
  }

  // ── PLAYING phase ─────────────────────────

  // Flap — read from bot.game.flapPressed
  // Set by state_manager when Sensor 1 tapped
  if (bot.game.flapPressed) {
    bot.game.flapPressed = false;
    birdVY = FLAP;
  }

  // Physics — unchanged from original
  birdVY += GRAVITY;
  birdY  += birdVY;
  pipeX  -= PIPE_SPEED;

  // Reset pipe when off screen
  if (pipeX + PIPE_W < 0) {
    pipeX    = 128;
    pipeGapY = random(8, 38);
    scored   = false;
  }

  // Score — when bird passes pipe
  if (!scored && pipeX < BIRD_X) {
    bot.game.score++;
    scored = true;

    // Update high score live
    if (bot.game.score > bot.game.highScore) {
      bot.game.highScore = bot.game.score;
    }

    Serial.print("Score: ");
    Serial.println(bot.game.score);
  }

  // Collision check
  if (_collide()) {
    bot.game.isDead  = true;
    gamePhase        = GAME_OVER;
    gameOverShownAt  = millis();

    Serial.print("Game Over! Score: ");
    Serial.println(bot.game.score);

    _drawGameOver(bot);
    return;
  }

  // Draw current frame
  _drawGame(bot);
}