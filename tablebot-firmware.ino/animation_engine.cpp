// ============================================
// animation_engine.cpp — TableBot
// Face expressions and animations
// Original face art by Pranav
// Integrated by Ribin — May 2026
// Changes from original:
//   - Removed setup() and loop()
//   - Removed own U8g2 instance — uses shared display
//   - Fixed randomSeed pin from 0 to 1
//   - Removed delay() from talkingFace
//   - Added Anim_tick() wrapper
//   - Added Animation_init()
//   - Connected to bot_state.h
// ============================================

#include "animation_engine.h"
#include "bot_state.h"
#include "pins.h"
#include <Arduino.h>

// ── EYE CENTRES ──────────────────────────────
#define FACE_LX  38
#define FACE_RX  90
#define FACE_EY  27

// ── PUPIL OFFSET ─────────────────────────────
static int           _pupilX   = 0;
static int           _pupilY   = 0;
static unsigned long _lastMove = 0;

// ── Talking face state (non-blocking) ────────
static int           _talkSeq[]    = {1,0,1,0,1,0,1,0,1,0,1,0};
static int           _talkStep     = 0;
static unsigned long _lastTalkStep = 0;
static bool          _talking      = false;

// ── Animation_init ───────────────────────────
// Call once in setup()
void Animation_init() {
  // GPIO 1 = PIN_BATTERY_ADC — safe ADC pin
  // GPIO 0 was original — strapping pin, unsafe
  randomSeed(analogRead(PIN_BATTERY_ADC));
}

// ══════════════════════════════════════════════
// INTERNAL HELPERS — unchanged from original
// ══════════════════════════════════════════════

void _updatePupil() {
  if (millis() - _lastMove >
      (unsigned long)random(1500, 3500)) {
    _lastMove = millis();
    _pupilX   = random(-3, 4);
    _pupilY   = random(-2, 3);
  }
}

void _drawEye(int cx, int cy, int radius,
              int blink, int pr, int px, int py) {
  display.setDrawColor(1);
  display.drawDisc(cx, cy, radius);

  display.setDrawColor(0);
  display.drawDisc(cx + px, cy + py, pr);

  display.setDrawColor(1);
  display.drawPixel(cx + px - 2, cy + py - 2);
  display.drawPixel(cx + px - 1, cy + py - 2);

  if (blink == 1) {
    display.setDrawColor(0);
    display.drawBox(cx - radius, cy - radius - 1,
                    radius * 2 + 1, radius);
  } else if (blink == 2) {
    display.setDrawColor(0);
    display.drawBox(cx - radius, cy - radius - 1,
                    radius * 2 + 1, radius * 2 + 2);
    display.setDrawColor(1);
    display.drawLine(cx - radius + 2, cy,
                     cx + radius - 2, cy);
  }
}

void _drawBrow(int cx, int cy, int style) {
  display.setDrawColor(1);
  switch (style) {
    case 0:
      display.drawLine(cx-9, cy,   cx,   cy-3);
      display.drawLine(cx,   cy-3, cx+9, cy  ); break;
    case 1:
      display.drawLine(cx-9, cy-2, cx,   cy-5);
      display.drawLine(cx,   cy-5, cx+9, cy-2); break;
    case 2:
      display.drawLine(cx-9, cy-2, cx+9, cy+4); break;
    case 3:
      display.drawLine(cx-9, cy+4, cx+9, cy-2); break;
    case 4:
      display.drawLine(cx-9, cy+3, cx+9, cy-1); break;
    case 5:
      display.drawLine(cx-9, cy-1, cx+9, cy+3); break;
    case 6:
      display.drawLine(cx-9, cy-5, cx,   cy-8);
      display.drawLine(cx,   cy-8, cx+9, cy-5); break;
  }
}

void _drawFrame() {
  display.setDrawColor(1);
  display.drawRFrame(0, 0, 128, 64, 8);
}

void _mouthSmile() {
  display.setDrawColor(1);
  display.drawLine(44,47, 52,54);
  display.drawLine(52,54, 64,57);
  display.drawLine(64,57, 76,54);
  display.drawLine(76,54, 84,47);
}

void _mouthFrown() {
  display.setDrawColor(1);
  display.drawLine(44,56, 52,49);
  display.drawLine(52,49, 64,46);
  display.drawLine(64,46, 76,49);
  display.drawLine(76,49, 84,56);
}

void _mouthFlat() {
  display.setDrawColor(1);
  display.drawLine(46,51, 82,51);
  display.drawLine(46,52, 82,52);
}

void _mouthSmallO() {
  display.setDrawColor(1);
  display.drawDisc(64, 52, 5);
  display.setDrawColor(0);
  display.drawDisc(64, 52, 2);
  display.setDrawColor(1);
}

void _mouthBigO() {
  display.setDrawColor(1);
  display.drawDisc(64, 51, 8);
  display.setDrawColor(0);
  display.drawDisc(64, 51, 4);
  display.setDrawColor(1);
}

void _mouthExcited() {
  display.setDrawColor(1);
  display.drawLine(38,46, 48,55);
  display.drawLine(48,55, 64,59);
  display.drawLine(64,59, 80,55);
  display.drawLine(80,55, 90,46);
}

void _mouthWavy() {
  display.setDrawColor(1);
  display.drawLine(42,52, 50,56);
  display.drawLine(50,56, 58,50);
  display.drawLine(58,50, 66,54);
  display.drawLine(66,54, 74,50);
  display.drawLine(74,50, 82,54);
}

void _mouthOpen() {
  display.setDrawColor(1);
  display.drawDisc(64, 50, 10);
  display.setDrawColor(0);
  display.drawDisc(64, 50, 6);
  display.setDrawColor(1);
}

void _mouthClosed() {
  display.setDrawColor(1);
  display.drawDisc(64, 50, 9);
  display.setDrawColor(0);
  display.drawDisc(64, 50, 6);
  display.setDrawColor(1);
  display.drawLine(55, 50, 73, 50);
  display.drawLine(55, 51, 73, 51);
}

// ══════════════════════════════════════════════
// PUBLIC FACE FUNCTIONS — unchanged from original
// ══════════════════════════════════════════════

void happyFace() {
  _updatePupil();
  display.clearBuffer();
  _drawFrame();
  _drawBrow(FACE_LX, 15, 1);
  _drawBrow(FACE_RX, 15, 1);
  _drawEye(FACE_LX, FACE_EY, 11, 0, 4, _pupilX, _pupilY);
  _drawEye(FACE_RX, FACE_EY, 11, 0, 4, _pupilX, _pupilY);
  _mouthSmile();
 // display.sendBuffer();
}

void sadFace() {
  _updatePupil();
  display.clearBuffer();
  _drawFrame();
  _drawBrow(FACE_LX, 15, 4);
  _drawBrow(FACE_RX, 15, 5);
  _drawEye(FACE_LX, FACE_EY, 10, 0, 3, 0, 2);
  _drawEye(FACE_RX, FACE_EY, 10, 0, 3, 0, 2);
  display.setDrawColor(1);
  display.drawLine(FACE_RX+6, 37, FACE_RX+6, 46);
  display.drawDisc(FACE_RX+6, 48, 2);
  _mouthFrown();
  //display.sendBuffer();
}

void angryFace() {
  _updatePupil();
  display.clearBuffer();
  _drawFrame();
  _drawBrow(FACE_LX, 17, 2);
  _drawBrow(FACE_RX, 17, 3);
  display.setDrawColor(0);
  display.drawBox(FACE_LX-12, 7, 25, 10);
  display.drawBox(FACE_RX-12, 7, 25, 10);
  display.setDrawColor(1);
  _drawBrow(FACE_LX, 17, 2);
  _drawBrow(FACE_RX, 17, 3);
  _drawEye(FACE_LX, FACE_EY+2, 9, 0, 3, _pupilX, _pupilY);
  _drawEye(FACE_RX, FACE_EY+2, 9, 0, 3, _pupilX, _pupilY);
  _mouthFlat();
  //display.sendBuffer();
}

void sleepyFace() {
  display.clearBuffer();
  _drawFrame();
  _drawBrow(FACE_LX, 15, 4);
  _drawBrow(FACE_RX, 15, 5);
  _drawEye(FACE_LX, FACE_EY, 10, 1, 3, 0, 0);
  _drawEye(FACE_RX, FACE_EY, 10, 1, 3, 0, 0);
  _mouthSmallO();
  display.setFont(u8g2_font_5x7_tf);
  display.setDrawColor(1);
  display.drawStr(97, 15, "z");
  display.drawStr(104, 9,  "Z");
  display.drawStr(111, 4,  "Z");
  //display.sendBuffer();
}

void excitedFace() {
  _updatePupil();
  display.clearBuffer();
  _drawFrame();
  _drawBrow(FACE_LX, 12, 6);
  _drawBrow(FACE_RX, 12, 6);
  _drawEye(FACE_LX, FACE_EY, 11, 0, 5, _pupilX, _pupilY);
  _drawEye(FACE_RX, FACE_EY, 11, 0, 5, _pupilX, _pupilY);
  _mouthExcited();
  //display.sendBuffer();
}

void surprisedFace() {
  display.clearBuffer();
  _drawFrame();
  _drawBrow(FACE_LX, 11, 6);
  _drawBrow(FACE_RX, 11, 6);
  _drawEye(FACE_LX, FACE_EY, 12, 0, 5, 0, 0);
  _drawEye(FACE_RX, FACE_EY, 12, 0, 5, 0, 0);
  _mouthBigO();
  //display.sendBuffer();
}

void confusedFace() {
  display.clearBuffer();
  _drawFrame();
  _drawBrow(FACE_LX, 11, 6);
  _drawBrow(FACE_RX, 16, 0);
  _drawEye(FACE_LX, FACE_EY, 10, 0, 3,  2, 0);
  _drawEye(FACE_RX, FACE_EY, 10, 0, 3, -2, 0);
  _mouthWavy();
  display.setFont(u8g2_font_5x7_tf);
  display.setDrawColor(1);
  display.drawStr(110, 20, "?");
  //display.sendBuffer();
}

// ── talkingFace — non-blocking version ───────
// Original used delay(140) — replaced with millis
// Call repeatedly from loop — advances one step
// each call until sequence is done
void talkingFace() {
  if (!_talking) {
    _talking   = true;
    _talkStep  = 0;
    _lastTalkStep = millis();
  }

  if (_talking &&
      millis() - _lastTalkStep >= 140) {
    _lastTalkStep = millis();
    _updatePupil();
    display.clearBuffer();
    _drawFrame();
    _drawBrow(FACE_LX, 15, 0);
    _drawBrow(FACE_RX, 15, 0);
    _drawEye(FACE_LX, FACE_EY, 10, 0, 3,
             _pupilX, _pupilY);
    _drawEye(FACE_RX, FACE_EY, 10, 0, 3,
             _pupilX, _pupilY);
    if (_talkSeq[_talkStep]) _mouthOpen();
    else                      _mouthClosed();
   // display.sendBuffer();

    _talkStep++;
    if (_talkStep >= 12) {
      _talkStep = 0;
      _talking  = false;
    }
  }
}

// ── blinkAnimation — non-blocking version ────
// Original used delay() — replaced with millis
// Returns true when blink is complete
bool blinkAnimation() {
  static int           blinkStage    = 0;
  static bool          blinkOpen     = false;
  static unsigned long lastBlinkStep = 0;
  static bool          blinkDone     = true;

  // Start new blink
  if (blinkDone) {
    blinkDone  = false;
    blinkStage = 0;
    blinkOpen  = false;
    lastBlinkStep = millis();
  }

  unsigned long wait = (blinkStage == 2) ? 100 : 70;

  if (millis() - lastBlinkStep >= wait) {
    lastBlinkStep = millis();

    display.clearBuffer();
    _drawFrame();
    _drawBrow(FACE_LX, 15, 1);
    _drawBrow(FACE_RX, 15, 1);
    _drawEye(FACE_LX, FACE_EY, 11,
             blinkStage, 4, 0, 0);
    _drawEye(FACE_RX, FACE_EY, 11,
             blinkStage, 4, 0, 0);
    _mouthSmile();
    //display.sendBuffer();

    if (!blinkOpen) {
      blinkStage++;
      if (blinkStage > 2) {
        blinkOpen  = true;
        blinkStage = 1;
      }
    } else {
      blinkStage--;
      if (blinkStage < 0) {
        blinkDone  = true;
        return true;  // blink complete
      }
    }
  }
  return false;
}

// ══════════════════════════════════════════════
// Anim_tick — MAIN WRAPPER
// Called every 100ms from main.ino
// Reads targetExpression from bot.face
// Calls correct face function
// ══════════════════════════════════════════════
void Anim_tick(FaceState& face) {

  // Update current to target
  face.currentExpression = face.targetExpression;

  // Call correct face based on expression
  switch (face.targetExpression) {

    case FACE_HAPPY:
      if (face.isBlinking)
        blinkAnimation();
      else
        happyFace();
      break;

    case FACE_SAD:
      sadFace();
      break;

    case FACE_SLEEPY:
      sleepyFace();
      break;

    case FACE_FOCUSED:
      // Focused — calm neutral expression
      // No blinking, no movement
      if (face.isAnimating)
        confusedFace();
      break;

    case FACE_NEUTRAL:
      confusedFace();
      break;

    default:
      happyFace();
      break;
  }
}