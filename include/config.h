#pragma once

#include <Arduino.h>

// Everything a builder might want to tune lives here. Pin names are the XIAO
// silkscreen aliases, which resolve to the right GPIO on both the C3 and C6.

// --- Pins --------------------------------------------------------------------
const int MOTOR_PWM_PIN = D1;  // MAX14870 PWM
const int MOTOR_DIR_PIN = D2;  // MAX14870 DIR
const int PIXEL_PIN     = D6;  // NeoPixel data line (UART0 TX on both boards)

const int NUM_PIXELS = 9;

// --- Wi-Fi access point ------------------------------------------------------
// WPA2 needs a password of at least 8 characters. Set AP_PASSWORD to "" for an
// open network.
const char *const AP_SSID     = "MoireDisc";
const char *const AP_PASSWORD = "moiredisc";
const int         AP_CHANNEL  = 1;

// --- Motor -------------------------------------------------------------------
// 20 kHz is above hearing, so the motor doesn't whine at part throttle.
const uint32_t MOTOR_PWM_HZ   = 20000;
const uint8_t  MOTOR_PWM_BITS = 10;
const uint32_t MOTOR_PWM_MAX  = (1UL << MOTOR_PWM_BITS) - 1;

// LEDC runs from the 40 MHz crystal on both chips (LEDC_DEFAULT_CLK is
// LEDC_USE_XTAL_CLK wherever SOC_LEDC_SUPPORT_XTAL_CLOCK is set), so the
// highest frequency at a given resolution is 40 MHz / 2^bits.
const uint32_t LEDC_SRC_HZ = 40000000UL;
static_assert(MOTOR_PWM_HZ <= (LEDC_SRC_HZ >> MOTOR_PWM_BITS),
              "MOTOR_PWM_HZ is above the LEDC ceiling - lower MOTOR_PWM_BITS");

// Below this duty a DC motor under load just hums, so non-zero commands are
// mapped into [MOTOR_MIN_DUTY_PCT .. 100] and the lowest slider step still
// turns the disc. Raise it if 1 % doesn't start the motor, lower it if 1 % is
// already too fast.
const float MOTOR_MIN_DUTY_PCT = 12.0f;

// How fast the applied speed may change, in percent per second. A full
// reversal from +100 to -100 therefore takes 200 / MOTOR_RAMP_PCT_PER_S.
const float MOTOR_RAMP_PCT_PER_S = 50.0f;

// Flip if "positive" turns the disc the wrong way.
const bool MOTOR_DIR_INVERT = false;

// Sweep period limits, seconds per full sine cycle.
const float SWEEP_PERIOD_MIN_S = 4.0f;
const float SWEEP_PERIOD_MAX_S = 120.0f;

// --- LEDs --------------------------------------------------------------------
const uint8_t LED_BRIGHTNESS_MAX = 255;

// Rotation slider at full deflection, in laps of the ring per second. Kept low
// on purpose: nothing on this disc should flash or strobe.
const float RING_ROT_MAX_LAPS_PER_S = 0.5f;

// With "sync to motor" on, full motor speed turns the ring at this many laps/s.
const float RING_SYNC_LAPS_PER_S = 0.5f;

// Base hue drift for the Rainbow effect, seconds per full hue revolution.
const float RAINBOW_HUE_PERIOD_S = 30.0f;

// --- Timing ------------------------------------------------------------------
const unsigned long UPDATE_PERIOD_MS = 20;    // motor + LED tick, 50 Hz
const unsigned long SAVE_DELAY_MS    = 2000;  // NVS write after last change
