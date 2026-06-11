/ ============================================
// mode_manager.h — INNO TableBot Stage 2
// Handles BLE command parsing + mode switching
// Last updated: June 2026
// ============================================
 
#ifndef MODE_MANAGER_H
#define MODE_MANAGER_H
 
#include "bot_state.h"
 
// ── Init ─────────────────────────────────────
void Mode_init(BotState &bot);
 
// ── Called from loop when newCommandReceived ─
void Mode_apply(BotState &bot);
 
// ── Direct mode setters ──────────────────────
void Mode_setActive(BotState &bot);
void Mode_setFocus(BotState &bot);
void Mode_setGame(BotState &bot);
void Mode_setAI(BotState &bot);
void Mode_exitGame(BotState &bot);
 
// ── BLE command handlers (called by Mode_apply)
void Mode_handleAPIKey(BotState &bot, const char* key);
void Mode_handleVolume(BotState &bot, int vol);
void Mode_handleLocation(BotState &bot, float lat, float lon);
void Mode_handleAlarm(BotState &bot, int hour, int minute, const char* label);
void Mode_handleReminder(BotState &bot, int hour, int minute, const char* text);
 
#endif