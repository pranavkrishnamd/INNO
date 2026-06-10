// ============================================
// pins.h — INNO TableBot Stage 2 FINAL
// Board: Waveshare ESP32-S3-Tiny
// Last updated: June 2026
// All pins verified — directly accessible
// No soldering required
// ============================================

#ifndef PINS_H
#define PINS_H

// ── GC9A01 Round TFT Display (SPI) ──────────
// Protocol  : 4-wire SPI
// Voltage   : 3.3V
// Resolution: 240x240 round full color
// Library   : Arduino_GFX
// BLK       : hardwired to 3.3V (always on)
// All pins on top board right side ✅
#define PIN_TFT_SCL         10   // SPI Clock
#define PIN_TFT_SDA         11   // SPI MOSI
#define PIN_TFT_RES         12   // Reset
#define PIN_TFT_DC          13   // Data/Command

// ── Microphone INMP441 (I2S IN) ─────────────
// Protocol  : I2S input
// Voltage   : 3.3V
// Note      : L/R pin → GND (mono left)
// All pins on top board left side ✅
#define PIN_MIC_WS          5    // Word Select
#define PIN_MIC_SCK         6    // Bit Clock
#define PIN_MIC_SD          4    // Data in

// ── Amplifier MAX98357 (I2S OUT) ────────────
// Protocol  : I2S output
// Voltage   : 5V
// SD_MODE   : hardwired to 3.3V (always on)
// GP7 on top left, GP8+GP9 on top left/right ✅
#define PIN_AMP_BCLK        8    // Bit Clock
#define PIN_AMP_LRC         9    // Left/Right
#define PIN_AMP_DIN         7    // Data out

// ── Touch Sensor 1 — TTP223 (main) ──────────
// All bot functions + AI trigger
// Protocol  : Digital INPUT_PULLDOWN
// Voltage   : 3.3V
// GP2 on top board left side ✅
#define PIN_TOUCH           2

// ── Touch Sensor 2 — TTP223 (game only) ─────
// Enter game / exit game only
// Protocol  : Digital INPUT_PULLDOWN
// Voltage   : 3.3V
// GP14 on top board right side ✅
#define PIN_TOUCH_GAME      14

// ── Onboard WS2812 RGB LED ───────────────────
// BLE connection indicator
// Blue = connected, Off = disconnected
// Library   : Adafruit NeoPixel
// GP38 on bottom board right side ✅
#define PIN_WS2812          38
#define WS2812_COUNT        1

// ── External RGB LED (Battery indicator) ────
// Type      : XL-B504RGBW Common Anode
// Voltage   : 3.3V
// PCB       : 100 ohm resistor on each color pin
// Common Anode → hardwired to 3.3V
// Logic     : LOW = ON, HIGH = OFF (common anode)
// GP17,18 on top right, GP21 on bottom right ✅
// Colors:
//   Green  → battery > 60%
//   Orange → battery 20-60% (R+G together)
//   Red    → battery < 20% or charging
#define PIN_RGB_R           17
#define PIN_RGB_G           18
#define PIN_RGB_B           16

// ── Battery Monitor ──────────────────────────
// PCB: 100k+100k voltage divider
// BATTERY+ → midpoint → GP1 → GND
// ADC1 — safe alongside WiFi ✅
// GP1 on top board right side ✅
#define PIN_BATTERY_ADC     1

// ── TP4056 Charging Indicators ───────────────
// Both active LOW (LOW = event happening)
// GP39,40 on bottom board left side ✅
#define PIN_CHARGE_STATUS   39   // LOW = charging
#define PIN_CHARGE_FULL     40   // LOW = full

// ── UART Debug (NOT on PCB) ──────────────────
// TX/RX pins on top board ✅
#define PIN_UART_TX         43   // TX pin
#define PIN_UART_RX         44   // RX pin

// ── RESERVED — NEVER USE ─────────────────────
// GP0  → strapping pin
// GP3  → strapping pin
// GP19 → USB D-
// GP20 → USB D+
// GP26-GP32 → SPI flash internal
// GP33-GP37 → PSRAM internal
// GP45 → strapping pin
// GP46 → strapping pin
// GP48 → shares onboard LED (avoid)

// ── FREE PINS — available for expansion ──────
// GP15 → free (top board right)
// GP21 → free (top board right)
// GP41 → free (bottom board left)
// GP42 → free (bottom board left)
// GP47 → free (bottom board left)

// ── BLE & WiFi ───────────────────────────────
// Internal antenna — no GPIO required

// ── PIN SUMMARY FOR PCB TEAM ─────────────────
// All pins directly accessible — no soldering
//
// TOP BOARD LEFT SIDE:
// GP1  → Battery ADC (voltage divider midpoint)
// GP2  → Touch Sensor 1 (main, INPUT_PULLDOWN)
// GP4  → Mic SD (data in)
// GP5  → Mic WS (word select)
// GP6  → Mic SCK (bit clock)
// GP7  → Amp DIN (data out)
// GP8  → Amp BCLK (bit clock)
//
// TOP BOARD RIGHT SIDE:
// GP9  → Amp LRC (left/right clock)
// GP10 → TFT SCL (SPI clock)
// GP11 → TFT SDA (SPI MOSI)
// GP12 → TFT RES (reset)
// GP13 → TFT DC  (data/command)
// GP14 → Touch Sensor 2 (game, INPUT_PULLDOWN)
// GP17 → RGB LED Red   (100 ohm resistor)
// GP18 → RGB LED Green (100 ohm resistor)
// GP15 → FREE
// GP21 → FREE
//
// BOTTOM BOARD LEFT SIDE:
// GP39 → TP4056 CHRG (active LOW)
// GP40 → TP4056 STDBY (active LOW)
// GP41 → FREE
// GP42 → FREE
// GP47 → FREE
//
// BOTTOM BOARD RIGHT SIDE:
// GP21 → RGB LED Blue  (100 ohm resistor)
// GP38 → Onboard WS2812 (BLE indicator)
//
// TOP BOARD POWER:
// 5V, GND, 3V3 → power rails
// TX  → UART TX (debug only, no PCB route)
// RX  → UART RX (debug only, no PCB route)
//
// HARDWIRED:
// TFT BLK    → 3.3V direct
// AMP SD_MODE→ 3.3V direct
// RGB Anode  → 3.3V direct

#endif