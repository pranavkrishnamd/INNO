// ============================================
// display_manager.h — TableBot
// ============================================

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include "bot_state.h"
#include <U8g2lib.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>

extern U8G2_SH1106_128X64_NONAME_F_HW_I2C display;

void Display_init();
void Display_render(BotState& bot);
bool Weather_fetch(BotState& bot);

#endif