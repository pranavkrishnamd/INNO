// ============================================
// state_manager.h — TableBot
// Declares what the state manager can do.
// This is the menu — state_manager.cpp is
// where the actual cooking happens.
// ============================================

#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include "bot_state.h"

// Call this once at the start in setup()
void State_init(BotState& bot);

// Call this when touch sensor fires
// Handles: sleep→idle, idle→active, active→idle
void State_onTouch(BotState& bot);

// Call this every 5 seconds in main loop
// Checks if bot has been idle too long → sleep
void State_checkIdle(BotState& bot);

// Call this when WiFi or weather fails
// Switches to ERROR state, shows sad face
void State_onError(BotState& bot);

// Call this when error is recovered
// Switches back to IDLE
void State_onRecovery(BotState& bot);

// Call this when weather fetch starts
// Switches to UPDATING state briefly
void State_onUpdateStart(BotState& bot);

// Call this when weather fetch finishes
// Switches back to IDLE
void State_onUpdateDone(BotState& bot);

#endif