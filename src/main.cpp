#include <Arduino.h>
#include <WiFi.h>

#include "config.h"
#include "leds.h"
#include "motor.h"
#include "settings.h"
#include "web.h"

void setup() {
  Serial.begin(115200);

  // Serial is the native USB CDC. With no host draining it, writes would block
  // for up to the TX timeout each -- long enough to stall the tick and make the
  // lights stutter. Zero means "drop it if nobody is listening".
  Serial.setTxTimeoutMs(0);

  settingsLoad();
  motorSetup();
  ledsSetup();
  webSetup();

  Serial.printf("Moire disc ready. %d LEDs, PWM %lu Hz. Connect to \"%s\" and "
                "open http://192.168.4.1\n",
                NUM_PIXELS, (unsigned long)MOTOR_PWM_HZ, AP_SSID);
}

void loop() {
  static unsigned long lastUpdate = 0;

  webService();

  unsigned long now = millis();
  settingsService(now);

  if (now - lastUpdate < UPDATE_PERIOD_MS) return;
  float dt = (now - lastUpdate) / 1000.0f;
  lastUpdate = now;

  // A stall (flash write, slow client) would otherwise be replayed as one big
  // step in every phase. Capping dt turns it into a brief slow-down instead.
  if (dt > 0.1f) dt = 0.1f;

  motorUpdate(dt);
  ledsUpdate(dt, motorActual());

  static unsigned long lastPrint = 0;
  if (Serial && now - lastPrint >= 500) {
    lastPrint = now;
    Serial.printf("motor %s target %6.1f %% actual %6.1f %%\tfx %u\tclients %u\n",
                  settings.motorMode == MOTOR_SWEEP ? "SWEEP " : "MANUAL",
                  motorTarget(), motorActual(), settings.effect,
                  WiFi.softAPgetStationNum());
  }
}
