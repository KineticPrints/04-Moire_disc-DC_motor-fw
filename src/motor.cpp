#include "motor.h"

#include <Arduino.h>
#include <math.h>

#include "config.h"
#include "settings.h"

// The MAX14870 is driven sign-magnitude: DIR picks the sense, PWM duty the
// magnitude. Its EN pin is tied low on the board, so the bridge is always on
// and PWM low means the outputs brake.

static float target     = 0.0f;  // what the mode asks for
static float applied    = 0.0f;  // ramped value actually on the pins
static float sweepPhase = 0.0f;  // radians, integrated

// The one place that touches the motor pins.
static void writeMotor(float pct) {
  static bool forwardNow = true;

  float mag = fabsf(pct);
  if (mag < 1.0f) {
    ledcWrite(MOTOR_PWM_PIN, 0);
    return;
  }

  // updateRamp() never lets `applied` jump across zero, so a sign change only
  // ever arrives here after a tick at duty 0 -- DIR never flips under load.
  bool forward = pct > 0.0f;
  if (forward != forwardNow) {
    forwardNow = forward;
    digitalWrite(MOTOR_DIR_PIN, (forward != MOTOR_DIR_INVERT) ? HIGH : LOW);
  }

  if (mag > 100.0f) mag = 100.0f;
  float duty = MOTOR_MIN_DUTY_PCT + (100.0f - MOTOR_MIN_DUTY_PCT) * (mag - 1.0f) / 99.0f;
  ledcWrite(MOTOR_PWM_PIN, (uint32_t)(duty * MOTOR_PWM_MAX / 100.0f + 0.5f));
}

void motorSetup() {
  pinMode(MOTOR_DIR_PIN, OUTPUT);
  digitalWrite(MOTOR_DIR_PIN, MOTOR_DIR_INVERT ? LOW : HIGH);

  if (!ledcAttach(MOTOR_PWM_PIN, MOTOR_PWM_HZ, MOTOR_PWM_BITS)) {
    Serial.println("ERROR: motor PWM attach failed");
  }
  ledcWrite(MOTOR_PWM_PIN, 0);
}

static void updateTarget(float dt) {
  if (settings.motorMode == MOTOR_SWEEP) {
    // Integrating the phase (rather than evaluating sin(2*pi*t/T)) means
    // changing the period never makes the speed jump.
    float period = constrain((float)settings.sweepPeriod, SWEEP_PERIOD_MIN_S,
                             SWEEP_PERIOD_MAX_S);
    sweepPhase += 2.0f * (float)M_PI * dt / period;
    if (sweepPhase > 2.0f * (float)M_PI) sweepPhase -= 2.0f * (float)M_PI;

    float lo = settings.sweepMin, hi = settings.sweepMax;
    float center = (hi + lo) * 0.5f;
    float amp    = (hi - lo) * 0.5f;  // negative if min > max; still a sine
    target = center + amp * sinf(sweepPhase);
  } else {
    target = settings.speed;
  }
  target = constrain(target, -100.0f, 100.0f);
}

static void updateRamp(float dt) {
  float step = MOTOR_RAMP_PCT_PER_S * dt;
  float next = applied;
  if (target > applied)      next = min(applied + step, target);
  else if (target < applied) next = max(applied - step, target);

  // Stop at zero for one tick on the way through, so writeMotor() gets to cut
  // the PWM before it flips DIR.
  if ((applied > 0.0f && next < 0.0f) || (applied < 0.0f && next > 0.0f)) {
    next = 0.0f;
  }
  applied = next;
}

void motorUpdate(float dt) {
  updateTarget(dt);
  updateRamp(dt);
  writeMotor(applied);
}

float motorTarget() { return target; }
float motorActual() { return applied; }
