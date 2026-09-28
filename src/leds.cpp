#include "leds.h"

#include <Adafruit_NeoPixel.h>
#include <math.h>

#include "config.h"
#include "settings.h"

// Nothing here may flash. Every effect is a continuous function of slowly
// integrated phases, and switching effects crossfades instead of cutting.

static Adafruit_NeoPixel pixels(NUM_PIXELS, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

static const float TWO_PI_F      = 2.0f * (float)M_PI;
static const float CROSSFADE_S   = 0.8f;

// All phases are integrated from dt rather than derived from millis(), so a
// setting change alters the rate from here on without making anything jump.
static float ringPhase  = 0.0f;  // pattern offset, in pixels round the ring
static float hueDrift   = 0.0f;  // Rainbow's base hue, 0..65536
static float auroraT[3] = {0.0f, 0.0f, 0.0f};  // Aurora wave phases, radians

static uint32_t frame[NUM_PIXELS];     // current effect, pre-gamma
static uint32_t shown[NUM_PIXELS];     // what is on the ring, pre-gamma
static uint32_t fadeFrom[NUM_PIXELS];  // snapshot taken at an effect switch
static float    fadeLeft = 0.0f;       // seconds of crossfade remaining
static uint8_t  lastEffect = 0xFF;

static uint16_t hue16(float deg) { return (uint16_t)(deg * 65536.0f / 360.0f); }

static void wrap(float &x, float period) {
  while (x >= period) x -= period;
  while (x < 0.0f) x += period;
}

// --- Effects -----------------------------------------------------------------

// The pattern centre sits at ringPhase and the hue walks away from it
// symmetrically both ways round, so the two sides meet at the same hue
// opposite the centre and the pattern has no seam as it turns. Same maths as
// the stepper version's renderStrip().
static void fxGradient() {
  const float HALF   = NUM_PIXELS / 2.0f;
  uint16_t    hue    = hue16(settings.hue);
  float       spread = settings.spread * 65535.0f / 100.0f;
  for (int i = 0; i < NUM_PIXELS; i++) {
    float around = fmodf((float)i - ringPhase + NUM_PIXELS, (float)NUM_PIXELS);
    float d      = (around > HALF) ? (NUM_PIXELS - around) : around;
    frame[i] = pixels.ColorHSV(hue + (uint16_t)(spread * d / HALF), 255, 255);
  }
}

// One full rainbow round the ring, turning with ringPhase while the base hue
// drifts underneath it -- the old standalone look.
static void fxRainbow() {
  for (int i = 0; i < NUM_PIXELS; i++) {
    float pos = ((float)i - ringPhase) / NUM_PIXELS;
    // pos goes negative for pixels behind the centre. Converting a negative
    // float straight to uint16_t is undefined (it clamps to 0 = red on these
    // chips), so go through a signed int and let it wrap round the hue circle.
    int32_t h = (int32_t)(hueDrift + pos * 65536.0f);
    frame[i]  = pixels.ColorHSV((uint16_t)h, 255, 255);
  }
}

static void fxSolid() {
  uint32_t c = pixels.ColorHSV(hue16(settings.hue), 255, 255);
  for (int i = 0; i < NUM_PIXELS; i++) frame[i] = c;
}

// Soft colour clouds: two hue waves travelling opposite ways round the ring,
// plus a brightness wave that never drops below ~35 %. The periods are
// deliberately unrelated so the pattern takes a long time to repeat.
static void fxAurora() {
  float swing = settings.spread * 65535.0f / 100.0f * 0.5f;
  uint16_t base = hue16(settings.hue);
  for (int i = 0; i < NUM_PIXELS; i++) {
    float a = TWO_PI_F * ((float)i - ringPhase) / NUM_PIXELS;
    float h = 0.6f * sinf(a + auroraT[0]) + 0.4f * sinf(2.0f * a - auroraT[1]);
    float v = 0.675f + 0.325f * sinf(a - auroraT[2]);
    float s = 0.85f + 0.15f * sinf(2.0f * a + auroraT[0] - auroraT[2]);
    frame[i] = pixels.ColorHSV((uint16_t)(base + (int32_t)(h * swing)),
                               (uint8_t)(s * 255.0f), (uint8_t)(v * 255.0f));
  }
}

static void fxOff() {
  for (int i = 0; i < NUM_PIXELS; i++) frame[i] = 0;
}

// --- Output ------------------------------------------------------------------

static uint32_t blend(uint32_t a, uint32_t b, float t) {
  uint8_t out[3];
  for (int k = 0; k < 3; k++) {
    int ca = (a >> (16 - 8 * k)) & 0xFF;
    int cb = (b >> (16 - 8 * k)) & 0xFF;
    out[k] = (uint8_t)(ca + (cb - ca) * t + 0.5f);
  }
  return ((uint32_t)out[0] << 16) | ((uint32_t)out[1] << 8) | out[2];
}

void ledsSetup() {
  pixels.begin();
  pixels.clear();
  pixels.show();
}

void ledsUpdate(float dt, float motorPct) {
  float lapsPerS = settings.syncMotor
                       ? RING_SYNC_LAPS_PER_S * motorPct / 100.0f
                       : RING_ROT_MAX_LAPS_PER_S * settings.rotation / 100.0f;
  ringPhase += lapsPerS * NUM_PIXELS * dt;
  wrap(ringPhase, (float)NUM_PIXELS);

  hueDrift += 65536.0f * dt / RAINBOW_HUE_PERIOD_S;
  wrap(hueDrift, 65536.0f);

  auroraT[0] += TWO_PI_F * dt / 11.0f;
  auroraT[1] += TWO_PI_F * dt / 17.0f;
  auroraT[2] += TWO_PI_F * dt / 7.0f;
  for (float &p : auroraT) wrap(p, TWO_PI_F);

  if (settings.effect != lastEffect) {
    // Snapshot what is on the ring now -- mid-fade included -- and fade from
    // it. At boot `shown` is all zero, so the disc fades up from black.
    memcpy(fadeFrom, shown, sizeof(shown));
    fadeLeft   = CROSSFADE_S;
    lastEffect = settings.effect;
  }

  switch (settings.effect) {
    case FX_GRADIENT: fxGradient(); break;
    case FX_RAINBOW:  fxRainbow();  break;
    case FX_SOLID:    fxSolid();    break;
    case FX_AURORA:   fxAurora();   break;
    default:          fxOff();      break;
  }

  float t = 1.0f;
  if (fadeLeft > 0.0f) {
    fadeLeft -= dt;
    t = (fadeLeft > 0.0f) ? 1.0f - fadeLeft / CROSSFADE_S : 1.0f;
  }

  pixels.setBrightness((uint8_t)(settings.brightness * LED_BRIGHTNESS_MAX / 100));
  for (int i = 0; i < NUM_PIXELS; i++) {
    shown[i] = (t < 1.0f) ? blend(fadeFrom[i], frame[i], t) : frame[i];
    pixels.setPixelColor(i, pixels.gamma32(shown[i]));
  }
  pixels.show();
}
