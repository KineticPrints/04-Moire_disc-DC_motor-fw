#include "web.h"

#include <DNSServer.h>
#include <WebServer.h>
#include <WiFi.h>

#include "config.h"
#include "motor.h"
#include "settings.h"
#include "web_page.h"

// The disc is its own access point; the phone joins it and gets the control
// page. There is no internet behind it, so every DNS name resolves to us and
// phones show the page in their captive-portal popup automatically.

static WebServer server(80);
static DNSServer dns;
static IPAddress apIP(192, 168, 4, 1);

static void sendState() {
  char buf[400];
  snprintf(buf, sizeof(buf),
           "{\"motorMode\":%u,\"speed\":%d,\"sweepMin\":%d,\"sweepMax\":%d,"
           "\"sweepPeriod\":%u,\"effect\":%u,\"hue\":%u,\"spread\":%u,"
           "\"rotation\":%d,\"brightness\":%u,\"syncMotor\":%s,"
           "\"target\":%.1f,\"actual\":%.1f,\"clients\":%u,\"uptime\":%lu}",
           settings.motorMode, settings.speed, settings.sweepMin,
           settings.sweepMax, settings.sweepPeriod, settings.effect,
           settings.hue, settings.spread, settings.rotation,
           settings.brightness, settings.syncMotor ? "true" : "false",
           motorTarget(), motorActual(), WiFi.softAPgetStationNum(),
           millis() / 1000);
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", buf);
}

// Reads an integer query argument, clamps it into [lo, hi] and stores it.
// Returns true if the argument was present.
template <typename T>
static bool takeArg(const char *name, T &dst, long lo, long hi) {
  if (!server.hasArg(name)) return false;
  dst = (T)constrain(server.arg(name).toInt(), lo, hi);
  return true;
}

static void handleSet() {
  bool any = false;
  any |= takeArg("motorMode", settings.motorMode, 0, MOTOR_MODE_COUNT - 1);
  any |= takeArg("speed", settings.speed, -100, 100);
  any |= takeArg("sweepMin", settings.sweepMin, -100, 100);
  any |= takeArg("sweepMax", settings.sweepMax, -100, 100);
  any |= takeArg("sweepPeriod", settings.sweepPeriod, (long)SWEEP_PERIOD_MIN_S,
                 (long)SWEEP_PERIOD_MAX_S);
  any |= takeArg("effect", settings.effect, 0, FX_COUNT - 1);
  any |= takeArg("hue", settings.hue, 0, 359);
  any |= takeArg("spread", settings.spread, 0, 100);
  any |= takeArg("rotation", settings.rotation, -100, 100);
  any |= takeArg("brightness", settings.brightness, 0, 100);
  any |= takeArg("syncMotor", settings.syncMotor, 0, 1);
  if (any) settingsChanged();
  sendState();
}

static void handleReset() {
  settingsReset();
  sendState();
}

static void handlePage() {
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html", WEB_PAGE);
}

// Anything else -- including the OS connectivity checks
// (generate_204, hotspot-detect.html, ...) -- gets bounced to the page, which
// is what makes the phone open it as a captive portal.
static void handleNotFound() {
  server.sendHeader("Location", String("http://") + apIP.toString() + "/", true);
  server.send(302, "text/plain", "");
}

void webSetup() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  bool ok = WiFi.softAP(AP_SSID, (AP_PASSWORD[0] ? AP_PASSWORD : nullptr),
                        AP_CHANNEL);
  WiFi.setSleep(false);
  Serial.printf("AP \"%s\" %s at %s\n", AP_SSID, ok ? "up" : "FAILED",
                WiFi.softAPIP().toString().c_str());

  dns.setErrorReplyCode(DNSReplyCode::NoError);
  dns.start(53, "*", apIP);

  server.on("/", HTTP_GET, handlePage);
  server.on("/api/state", HTTP_GET, sendState);
  server.on("/api/set", HTTP_GET, handleSet);
  server.on("/api/reset", HTTP_GET, handleReset);
  server.onNotFound(handleNotFound);
  server.begin();
}

void webService() {
  dns.processNextRequest();
  server.handleClient();
}
