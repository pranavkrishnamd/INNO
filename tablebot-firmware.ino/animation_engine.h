// ============================================
// animation_engine.h — TableBot
// Face expressions and animations
// Original face art by Pranav
// Integrated by Ribin — May 2026
// ============================================

#ifndef ANIMATION_ENGINE_H
#define ANIMATION_ENGINE_H

#include "bot_state.h"
#include <U8g2lib.h>

// External display object — defined in display_manager.cpp
extern U8G2_SH1106_128X64_NONAME_F_HW_I2C display;

// ── Init — call once in setup() ──────────────
void Animation_init();

// ── Main wrapper — called every 100ms ────────
// Reads bot.face.targetExpression
// Calls the correct face function
void Anim_tick(FaceState& face);

// ── Individual face functions ─────────────────
void happyFace();
void sadFace();
void angryFace();
void sleepyFace();
void excitedFace();
void surprisedFace();
void confusedFace();
void talkingFace();
bool blinkAnimation();

#endif