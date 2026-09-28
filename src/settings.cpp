#include "settings.h"

#include <Arduino.h>
#include <Preferences.h>

#include "config.h"

// Bump whenever the Settings layout changes, so an old blob in flash is
// ignored instead of being read back as garbage.
static const uint8_t SETTINGS_VERSION = 1;

static const Settings DEFAULTS = {
    /* motorMode   */ MOTOR_MANUAL,
    /* speed       */ 30,
    /* sweepMin    */ 10,
    /* sweepMax    */ 60,
    /* sweepPeriod */ 30,
    /* effect      */ FX_RAINBOW,
    /* hue         */ 120,
    /* spread      */ 33,
    /* rotation    */ 8,
    /* brightness  */ 25,
    /* syncMotor   */ false,
};

Settings settings = DEFAULTS;

static Preferences   prefs;
static bool          dirty     = false;
static unsigned long changedMs = 0;

void settingsLoad() {
  settings = DEFAULTS;
  if (!prefs.begin("moire", true)) return;  // first boot: namespace absent

  Settings stored;
  if (prefs.getUChar("ver", 0) == SETTINGS_VERSION &&
      prefs.getBytes("s", &stored, sizeof(stored)) == sizeof(stored)) {
    settings = stored;
  }
  prefs.end();
}

void settingsReset() {
  settings = DEFAULTS;
  settingsChanged();
}

// Sliders send ~10 updates a second while dragged. Writing each one to flash
// would wear it for nothing, so the save waits until things have been quiet.
void settingsChanged() {
  dirty     = true;
  changedMs = millis();
}

void settingsService(unsigned long now) {
  if (!dirty || now - changedMs < SAVE_DELAY_MS) return;
  dirty = false;

  if (!prefs.begin("moire", false)) return;
  prefs.putUChar("ver", SETTINGS_VERSION);
  prefs.putBytes("s", &settings, sizeof(settings));
  prefs.end();
  Serial.println("Settings saved.");
}
