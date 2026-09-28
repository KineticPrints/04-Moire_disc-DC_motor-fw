#pragma once

#include <stdint.h>

enum MotorMode : uint8_t {
  MOTOR_MANUAL = 0,  // slider sets the speed directly
  MOTOR_SWEEP  = 1,  // sine between sweepMin and sweepMax
  MOTOR_MODE_COUNT,
};

enum LedEffect : uint8_t {
  FX_GRADIENT = 0,  // hue + spread, turning round the ring
  FX_RAINBOW  = 1,  // full rainbow, base hue drifting
  FX_SOLID    = 2,  // one colour everywhere
  FX_AURORA   = 3,  // slow wandering colour clouds
  FX_OFF      = 4,
  FX_COUNT,
};

// Everything the phone can change. Units are the ones the UI shows, so the web
// layer is a straight copy and the conversion lives next to the code using it.
struct Settings {
  uint8_t motorMode;    // MotorMode
  int8_t  speed;        // -100..100 %, MOTOR_MANUAL
  int8_t  sweepMin;     // -100..100 %, MOTOR_SWEEP
  int8_t  sweepMax;     // -100..100 %
  uint8_t sweepPeriod;  // seconds per cycle, SWEEP_PERIOD_MIN_S..MAX_S

  uint8_t  effect;      // LedEffect
  uint16_t hue;         // 0..359 degrees
  uint8_t  spread;      // 0..100 %, how far the hue travels across the ring
  int8_t   rotation;    // -100..100 %, ring rotation speed and sense
  uint8_t  brightness;  // 0..100 %
  bool     syncMotor;   // ring rotation follows the motor instead
};

extern Settings settings;

void settingsLoad();                    // NVS -> settings, defaults if absent
void settingsReset();                   // defaults, saved soon after
void settingsChanged();                 // schedule a save
void settingsService(unsigned long now);  // performs the delayed save
