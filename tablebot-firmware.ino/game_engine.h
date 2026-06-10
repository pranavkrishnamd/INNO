// ============================================
// game_engine.h — TableBot
// Flappy Bird game engine
// Original game code by Pranav
// Integrated by Ribin — May 2026
// ============================================

#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include "bot_state.h"
#include <U8g2lib.h>

// External display — defined in display_manager.cpp
extern U8G2_SH1106_128X64_NONAME_F_HW_I2C display;

// ── Init — call once in setup() ──────────────
void Game_init();

// ── Main tick — called every 20ms from main.ino
// Only runs when bot.mode == MODE_GAME
// Reads bot.game.flapPressed for input
// Writes bot.game.score, bot.game.isDead
void Game_tick(BotState& bot);

// ── Reset — call when entering game fresh ────
void Game_reset(BotState& bot);

#endif