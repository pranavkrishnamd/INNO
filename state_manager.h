// ============================================
// state_manager.h — INNO TableBot Stage 2
// System state transitions
// Last updated: June 2026
// ============================================
 
#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H
 
#include "bot_state.h"
 
// ── Init ─────────────────────────────────────
void State_init(BotState &bot);
 
// ── Called from loop ─────────────────────────
void State_checkIdle(BotState &bot);
void State_onTouch(BotState &bot);
 
// ── Weather / OTA transitions ────────────────
void State_onUpdateStart(BotState &bot);
void State_onUpdateDone(BotState &bot);
void State_onError(BotState &bot);
 
// ── AI state transitions ─────────────────────
void State_onListening(BotState &bot);
void State_onThinking(BotState &bot);
void State_onSpeaking(BotState &bot);
void State_onAIDone(BotState &bot);
 
// ── BLE send helper (defined in ble_manager) ─
extern void BLE_sendMessage(const char* msg);
 
#endif