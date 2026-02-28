#pragma once

#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Arduino_GFX_Library.h>
#include <math.h>

#define ISS_UPDATE_INTERVAL (30UL * 1000UL)  // 30 seconds

extern Arduino_GFX *gfx;

// ---------------------------------------------------------------------------
// HTTP helper
// ---------------------------------------------------------------------------
static String iss_https_get(const String &url) {
  Serial.printf("[ISS] GET %s\n", url.c_str());
  WiFiClientSecure *client = new WiFiClientSecure;
  if (!client) return "";
  client->setInsecure();
  String body;
  {
    HTTPClient https;
    https.begin(*client, url);
    https.setTimeout(15000);
    https.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    int code = https.GET();
    if (code == HTTP_CODE_OK) body = https.getString();
    else Serial.printf("[ISS] HTTP error: %d\n", code);
    https.end();
  }
  delete client;
  return body;
}

// ---------------------------------------------------------------------------
// Haversine great-circle distance in km (surface only — ignores altitude)
// ---------------------------------------------------------------------------
static float iss_haversine(float lat1, float lon1, float lat2, float lon2) {
  const float R  = 6371.0f;
  float dLat = (lat2 - lat1) * (float)M_PI / 180.0f;
  float dLon = (lon2 - lon1) * (float)M_PI / 180.0f;
  float a = sinf(dLat / 2) * sinf(dLat / 2)
          + cosf(lat1 * (float)M_PI / 180.0f)
          * cosf(lat2 * (float)M_PI / 180.0f)
          * sinf(dLon / 2) * sinf(dLon / 2);
  return R * 2.0f * atan2f(sqrtf(a), sqrtf(1.0f - a));
}

// Forward bearing in degrees (0 = N, 90 = E, …) from point 1 → point 2
static float iss_bearing(float lat1, float lon1, float lat2, float lon2) {
  float l1 = lat1 * (float)M_PI / 180.0f;
  float l2 = lat2 * (float)M_PI / 180.0f;
  float dl = (lon2 - lon1) * (float)M_PI / 180.0f;
  float x  = sinf(dl) * cosf(l2);
  float y  = cosf(l1) * sinf(l2) - sinf(l1) * cosf(l2) * cosf(dl);
  return fmodf(atan2f(x, y) * 180.0f / (float)M_PI + 360.0f, 360.0f);
}

static const char *iss_compass(float b) {
  const char *d[] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW", "N" };
  return d[(int)((b + 22.5f) / 45.0f) % 8];
}

// ---------------------------------------------------------------------------
// Fetch live ISS position and draw on screen.
// API: https://api.wheretheiss.at/v1/satellites/25544  (free, no key, HTTPS)
// Returns true on success.
// ---------------------------------------------------------------------------
bool issFetchAndDisplay(const char *userLat, const char *userLon) {
  String body = iss_https_get("https://api.wheretheiss.at/v1/satellites/25544");
  if (body.isEmpty()) return false;

  DynamicJsonDocument doc(1024);
  if (deserializeJson(doc, body)) {
    Serial.println("[ISS] JSON parse failed");
    return false;
  }

  float  issLat = doc["latitude"]  | 0.0f;
  float  issLon = doc["longitude"] | 0.0f;
  float  issAlt = doc["altitude"]  | 0.0f;   // km
  float  issVel = doc["velocity"]  | 0.0f;   // km/h
  String vis    = doc["visibility"] | "unknown";

  float uLat = atof(userLat);
  float uLon = atof(userLon);

  float surfDist = iss_haversine(uLat, uLon, issLat, issLon);
  // Slant range: straight-line from ground to ISS (accounting for altitude)
  float slantDist = sqrtf(surfDist * surfDist + issAlt * issAlt);
  float brng = iss_bearing(uLat, uLon, issLat, issLon);

  // ── Draw ──────────────────────────────────────────────────────────────────
  gfx->fillRect(0, 20, gfx->width(), gfx->height() - 20, RGB565_BLACK);

  // Sub-header bar
  gfx->fillRect(0, 20, gfx->width(), 12, 0x0841);
  gfx->setTextColor(0x07FF);
  gfx->setTextSize(1);
  gfx->setCursor(4, 22);
  gfx->print("ISS Live Tracker");

  // Visibility badge in top-right: VISIBLE=green, DAYLIGHT=yellow, ECLIPSED=gray
  String visUpper = vis;
  visUpper.toUpperCase();
  uint16_t visColor = (vis == "visible") ? 0x07E0 :
                      (vis == "daylight") ? 0xFFE0 : 0x7BEF;
  gfx->setTextColor(visColor);
  gfx->setCursor(gfx->width() - (int)visUpper.length() * 6 - 4, 22);
  gfx->print(visUpper);

  int y = 36;

  // ── ISS header ───────────────────────────────────────────────────────────
  gfx->setTextColor(0x07FF);
  gfx->setTextSize(2);
  gfx->setCursor(4, y);
  gfx->print("ISS Position");
  y += 20;

  // Lat / Lon
  char buf[56];
  gfx->setTextColor(RGB565_WHITE);
  gfx->setTextSize(1);
  snprintf(buf, sizeof(buf), "Lat: %+.2f    Lon: %+.2f", issLat, issLon);
  gfx->setCursor(4, y); gfx->print(buf);
  y += 12;

  // Altitude + velocity
  gfx->setTextColor(0xFD20);
  snprintf(buf, sizeof(buf), "Alt: %.0f km    Vel: %.0f km/h", issAlt, issVel);
  gfx->setCursor(4, y); gfx->print(buf);
  y += 14;

  // Divider
  gfx->drawFastHLine(0, y, gfx->width(), 0x2104);
  y += 6;

  // ── Distance from user ────────────────────────────────────────────────────
  gfx->setTextColor(0xFFE0);
  gfx->setTextSize(2);
  snprintf(buf, sizeof(buf), "%.0f km", slantDist);
  gfx->setCursor(4, y); gfx->print(buf);
  y += 22;

  gfx->setTextColor(0x7BEF);
  gfx->setTextSize(1);
  gfx->setCursor(4, y);
  gfx->print("line-of-sight from your location");
  y += 12;

  // Bearing
  gfx->setTextColor(0x07FF);
  snprintf(buf, sizeof(buf), "Bearing: %.0f%c (%s)", brng, 176, iss_compass(brng));
  gfx->setCursor(4, y); gfx->print(buf);
  y += 14;

  // Divider
  gfx->drawFastHLine(0, y, gfx->width(), 0x2104);
  y += 5;

  // ── Visibility explanation ────────────────────────────────────────────────
  gfx->setTextColor(visColor);
  gfx->setCursor(4, y);
  if      (vis == "visible")  gfx->print("ISS visible from your location now!");
  else if (vis == "daylight") gfx->print("ISS in daylight (not in Earth shadow)");
  else                        gfx->print("ISS in Earth shadow (eclipsed)");
  y += 12;

  gfx->setTextColor(0x7BEF);
  gfx->setCursor(4, y);
  gfx->print("Updates every 30 seconds");

  Serial.printf("[ISS] Lat=%.2f Lon=%.2f Alt=%.0fkm Dist=%.0fkm Bear=%.0f Vis=%s\n",
                issLat, issLon, issAlt, slantDist, brng, vis.c_str());
  return true;
}
