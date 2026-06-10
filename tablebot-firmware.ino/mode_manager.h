// ============================================
// mode_manager.h — TableBot
// Updated: added Mode_setGame declaration
// ============================================

#ifndef MODE_MANAGER_H
#define MODE_MANAGER_H

#include "bot_state.h"

void Mode_init(BotState& bot);
void Mode_apply(BotState& bot);
void Mode_setIdle(BotState& bot);
void Mode_setFocus(BotState& bot);
void Mode_setGame(BotState& bot);   // NEW
void Mode_exitGame(BotState& bot);  // NEW

#endif