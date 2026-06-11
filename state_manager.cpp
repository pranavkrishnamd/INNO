// ============================================
// state_manager.cpp — INNO TableBot Stage 2
// System state transitions
// Last updated: June 2026
// ============================================

#include "state_manager.h"
#include "mode_manager.h"
#include <Arduino.h>

// Idle timeout — 2 minutes of no touch
#define IDLE_TIMEOUT_MS   120000

// ════════════════════════════════════════════
// State_init
// Called once in setup() after BLE + WiFi
// ════════════════════════════════════════════
void State_init(BotState &bot) {
  bot.state                  = STATE_ACTIVE;
  bot.face.currentExpression = FACE_HAPPY;
  bot.face.targetExpression  = FACE_HAPPY;
  bot.face.showMouth         = false;
  bot.face.isBlinking        = false;
  bot.face.isAnimating       = false;
  bot.face.frame             = 0;
  bot.face.lastBlinkMs       = millis();
  bot.face.lastMorphMs       = millis();
  bot.lastTouchMs            = millis();
  Serial.println("State: Initialized → ACTIVE");
}

// ════════════════════════════════════════════
// State_checkIdle
// Called every 5s from loop
// Sleeping state removed per project spec
// Just resets to ACTIVE if stuck in error
// ════════════════════════════════════════════
void State_checkIdle(BotState &bot) {
  // Don't interfere with game or AI pipeline
  if (bot.mode == MODE_GAME) return;
  if (bot.state == STATE_LISTENING) return;
  if (bot.state == STATE_THINKING)  return;
  if (bot.state == STATE_SPEAKING)  return;

  // Auto-recover from error after idle timeout
  if (bot.state == STATE_ERROR) {
    unsigned long now = millis();
    if (now - bot.lastTouchMs > IDLE_TIMEOUT_MS) {
      Serial.println("State: Error timeout → ACTIVE");
      Mode_setActive(bot);
    }
  }
}

// ════════════════════════════════════════════
// State_onTouch
// Touch Sensor 1 — main touch
// Behavior depends on current mode + state
// ════════════════════════════════════════════
void State_onTouch(BotState &bot) {
  bot.lastTouchMs = millis();

  // ── AI mode touch logic ──────────────────
  if (bot.mode == MODE_AI) {
    if (bot.state == STATE_ACTIVE) {
      // Start recording
      State_onListening(bot);
      return;
    }
    if (bot.state == STATE_LISTENING) {
      // Stop recording → send to Gemini
      State_onThinking(bot);
      return;
    }
    if (bot.state == STATE_SPEAKING) {
      // Interrupt playback → back to idle
      State_onAIDone(bot);
      return;
    }
    return;
  }

  // ── Alarm dismiss ────────────────────────
  if (bot.alarm.isRinging) {
    bot.alarm.isRinging        = false;
    bot.alarm.isSet            = false;
    bot.face.currentExpression = FACE_HAPPY;
    bot.face.showMouth         = false;
    Serial.println("State: Alarm dismissed");
    return;
  }

  // ── Reminder dismiss ─────────────────────
  for (int i = 0; i < 5; i++) {
    if (bot.reminders[i].isShowing) {
      bot.reminders[i].isShowing = false;
      bot.reminders[i].isSet     = false;
      Serial.print("State: Reminder[");
      Serial.print(i);
      Serial.println("] dismissed");
      return;
    }
  }

  // ── Active / Focus — touch does nothing ──
  // (mode switching is BLE only)
  Serial.println("State: Touch — no action");
}

// ════════════════════════════════════════════
// State_onUpdateStart
// Called before weather fetch / OTA
// ════════════════════════════════════════════
void State_onUpdateStart(BotState &bot) {
  bot.state = STATE_UPDATING;
  Serial.println("State: → UPDATING");
}

// ════════════════════════════════════════════
// State_onUpdateDone
// Called after successful weather fetch / OTA
// ════════════════════════════════════════════
void State_onUpdateDone(BotState &bot) {
  bot.state = STATE_ACTIVE;
  Serial.println("State: → ACTIVE (update done)");
}

// ════════════════════════════════════════════
// State_onError
// Called on weather fail or system error
// Shows FACE_SAD, sends BLE status
// ════════════════════════════════════════════
void State_onError(BotState &bot) {
  bot.state                  = STATE_ERROR;
  bot.face.currentExpression = FACE_SAD;
  bot.face.targetExpression  = FACE_SAD;
  bot.face.showMouth         = false;

  if (bot.ble.isConnected) {
    BLE_sendMessage("STATE:ERROR");
  }
  Serial.println("State: → ERROR");
}

// ════════════════════════════════════════════
// AI STATE TRANSITIONS
// Called by ai_module when pipeline progresses
// ════════════════════════════════════════════

void State_onListening(BotState &bot) {
  bot.state                  = STATE_LISTENING;
  bot.face.currentExpression = FACE_LISTENING;
  bot.face.targetExpression  = FACE_LISTENING;
  bot.face.showMouth         = false;
  bot.ai.isRecording         = true;
  bot.ai.recordStart         = millis();

  if (bot.ble.isConnected) {
    BLE_sendMessage("STATE:LISTENING");
  }
  Serial.println("State: → LISTENING");
}

void State_onThinking(BotState &bot) {
  bot.state                  = STATE_THINKING;
  bot.face.currentExpression = FACE_THINKING;
  bot.face.targetExpression  = FACE_THINKING;
  bot.face.showMouth         = false;
  bot.ai.isRecording         = false;
  bot.ai.isWaitingResponse   = true;

  if (bot.ble.isConnected) {
    BLE_sendMessage("STATE:THINKING");
  }
  Serial.println("State: → THINKING");
}

void State_onSpeaking(BotState &bot) {
  bot.state                  = STATE_SPEAKING;
  bot.face.currentExpression = FACE_SPEAKING;
  bot.face.targetExpression  = FACE_SPEAKING;
  bot.face.showMouth         = true;   // ONLY place showMouth = true
  bot.ai.isWaitingResponse   = false;

  if (bot.ble.isConnected) {
    BLE_sendMessage("STATE:SPEAKING");
  }
  Serial.println("State: → SPEAKING");
}

void State_onAIDone(BotState &bot) {
  bot.state                  = STATE_ACTIVE;
  bot.face.currentExpression = FACE_HAPPY;
  bot.face.targetExpression  = FACE_HAPPY;
  bot.face.showMouth         = false;  // mouth off
  bot.ai.isRecording         = false;
  bot.ai.isWaitingResponse   = false;
  bot.ai.hasResponse         = false;

  if (bot.ble.isConnected) {
    BLE_sendMessage("STATE:ACTIVE");
  }
  Serial.println("State: AI done → ACTIVE");
}