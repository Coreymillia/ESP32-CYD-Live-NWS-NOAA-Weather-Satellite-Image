#pragma once

#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Arduino_GFX_Library.h>
#include <math.h>

#define SW_UPDATE_INTERVAL (15UL * 60UL * 1000UL)  // 15 minutes

extern Arduino_GFX *gfx;

// ---------------------------------------------------------------------------
// HTTP helper (SWPC endpoints require no auth, just setInsecure)
// ---------------------------------------------------------------------------
static String sw_https_get(const String &url) {
  Serial.printf("[SW] GET %s\n", url.c_str());
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
    else Serial.printf("[SW] HTTP error: %d\n", code);
    https.end();
  }
  delete client;
  return body;
}

// ---------------------------------------------------------------------------
// Extract last data row from a NOAA array-of-arrays JSON body.
// Avoids parsing the full document (which can have 60+ rows and overflow
// a small DynamicJsonDocument).  Returns e.g. `["2026-..","3.33","18","8"]`
// ---------------------------------------------------------------------------
static String sw_last_row_str(const String &body) {
  int end = body.lastIndexOf("]]");
  if (end < 0) return "";
  int start = body.lastIndexOf("[", end - 1);
  if (start < 0) return "";
  return body.substring(start, end + 1);
}

// ---------------------------------------------------------------------------
// Color for Kp / G-level: green → yellow → orange → red
// ---------------------------------------------------------------------------
static uint16_t sw_kp_color(float kp) {
  if (kp < 4.0f) return 0x07E0;  // green   — quiet
  if (kp < 5.0f) return 0xFFE0;  // yellow  — unsettled
  if (kp < 6.0f) return 0xFD20;  // orange  — G1
  if (kp < 7.0f) return 0xFB00;  // d-orange — G2
  return 0xF800;                  // red     — G3+
}

// Lowest latitude (degrees N) where aurora may be visible at this Kp
// Empirical: 77 − (kp × 3.5)
static float sw_aurora_lat(float kp) {
  return 77.0f - kp * 3.5f;
}

// ---------------------------------------------------------------------------
// Fetch Kp index, solar wind speed, and Bz — draw on screen.
// Returns true on success (at least Kp was fetched).
// ---------------------------------------------------------------------------
bool swFetchAndDisplay(const char *lat, const char *lon) {

  // ── 1. Kp index (3-hour planetary) ───────────────────────────────────────
  float  kpVal  = -1.0f;
  String kpTime = "";
  {
    String body = sw_https_get(
      "https://services.swpc.noaa.gov/products/noaa-planetary-k-index.json");
    if (!body.isEmpty()) {
      String rowStr = sw_last_row_str(body);
      if (!rowStr.isEmpty()) {
        DynamicJsonDocument doc(256);
        if (!deserializeJson(doc, rowStr)) {
          JsonArray row = doc.as<JsonArray>();
          const char *ks = row[1] | "0";
          kpVal = String(ks).toFloat();
          String ts = String(row[0] | "");
          if (ts.length() >= 16) kpTime = ts.substring(11, 16);  // "HH:MM"
        }
      }
    }
  }

  // ── 2. Solar wind plasma — speed (km/s) ──────────────────────────────────
  float swSpeed = -1.0f;
  {
    String body = sw_https_get(
      "https://services.swpc.noaa.gov/products/solar-wind/plasma-5-minute.json");
    if (!body.isEmpty()) {
      String rowStr = sw_last_row_str(body);
      if (!rowStr.isEmpty()) {
        DynamicJsonDocument doc(256);
        if (!deserializeJson(doc, rowStr)) {
          JsonArray row = doc.as<JsonArray>();
          const char *vs = row[2] | "-1";
          swSpeed = String(vs).toFloat();
        }
      }
    }
  }

  // ── 3. Solar wind magnetic field — Bz and Bt (nT) ────────────────────────
  float bzVal = 999.0f;   // sentinel
  float btVal = 0.0f;
  {
    String body = sw_https_get(
      "https://services.swpc.noaa.gov/products/solar-wind/mag-5-minute.json");
    if (!body.isEmpty()) {
      String rowStr = sw_last_row_str(body);
      if (!rowStr.isEmpty()) {
        DynamicJsonDocument doc(384);
        if (!deserializeJson(doc, rowStr)) {
          JsonArray row = doc.as<JsonArray>();
          const char *bzs = row[3] | "999";
          const char *bts = row[6] | "0";
          bzVal = String(bzs).toFloat();
          btVal = String(bts).toFloat();
        }
      }
    }
  }

  if (kpVal < 0) return false;  // Kp is the essential field

  // ── Draw ──────────────────────────────────────────────────────────────────
  gfx->fillRect(0, 20, gfx->width(), gfx->height() - 20, RGB565_BLACK);

  // Sub-header bar
  gfx->fillRect(0, 20, gfx->width(), 12, 0x0841);
  gfx->setTextColor(0xFD20);
  gfx->setTextSize(1);
  gfx->setCursor(4, 22);
  gfx->print("NOAA Space Weather");
  if (!kpTime.isEmpty()) {
    String ts = "Kp@" + kpTime + " UTC";
    gfx->setTextColor(0x7BEF);
    gfx->setCursor(gfx->width() - (int)ts.length() * 6 - 4, 22);
    gfx->print(ts);
  }

  int y = 36;

  // ── Kp + G-storm level ───────────────────────────────────────────────────
  uint16_t kpColor = sw_kp_color(kpVal);
  gfx->setTextColor(kpColor);
  gfx->setTextSize(3);
  char kpStr[16];
  snprintf(kpStr, sizeof(kpStr), "Kp %.1f", kpVal);
  gfx->setCursor(4, y);
  gfx->print(kpStr);

  // G-level badge: derive from kp
  const char *gLabel = "G0";
  if      (kpVal >= 9.0f) gLabel = "G5";
  else if (kpVal >= 8.0f) gLabel = "G4";
  else if (kpVal >= 7.0f) gLabel = "G3";
  else if (kpVal >= 6.0f) gLabel = "G2";
  else if (kpVal >= 5.0f) gLabel = "G1";
  gfx->setTextSize(2);
  int gx = 4 + 10 * 18;  // after "Kp X.X" at textSize 3 (18px/char)
  gfx->setTextColor(kpColor);
  gfx->setCursor(gx, y + 6);
  gfx->print(gLabel);
  y += 30;

  // ── Aurora visibility estimate ───────────────────────────────────────────
  gfx->setTextSize(1);
  float auroraLat = sw_aurora_lat(kpVal);
  if (kpVal < 4.0f) {
    gfx->setTextColor(0x07E0);
    gfx->setCursor(4, y);
    gfx->print("Aurora: quiet  (Kp >= 4 needed)");
    y += 11;
  } else {
    gfx->setTextColor(0x07FF);
    char msg[48];
    snprintf(msg, sizeof(msg), "Aurora possible above %.0f%cN", auroraLat, 176);
    gfx->setCursor(4, y); gfx->print(msg);
    y += 11;
    // Check against user's saved latitude
    float userLat = atof(lat);
    if (userLat >= auroraLat) {
      gfx->setTextColor(0x07E0);
      gfx->setCursor(4, y);
      gfx->print(">>> Possibly visible at your lat!");
    } else {
      gfx->setTextColor(0x7BEF);
      char dist[44];
      snprintf(dist, sizeof(dist), "Your lat %.1f%cN (need >= %.0f%cN)",
               userLat, 176, auroraLat, 176);
      gfx->setCursor(4, y); gfx->print(dist);
    }
    y += 11;
  }

  // Divider
  gfx->drawFastHLine(0, y, gfx->width(), 0x2104);
  y += 5;

  // ── Solar wind speed ─────────────────────────────────────────────────────
  gfx->setTextColor(0xFD20);
  gfx->setTextSize(1);
  gfx->setCursor(4, y); gfx->print("Solar Wind:");
  y += 11;

  if (swSpeed > 0) {
    uint16_t sc = (swSpeed > 600) ? 0xF800 : (swSpeed > 450) ? 0xFFE0 : 0x07E0;
    gfx->setTextColor(sc);
    char buf[40];
    snprintf(buf, sizeof(buf), "Speed: %.0f km/s", swSpeed);
    gfx->setCursor(4, y); gfx->print(buf);
    y += 11;
  }

  // ── Bz / Bt ──────────────────────────────────────────────────────────────
  if (bzVal < 900.0f) {
    // Bz: negative (southward) = aurora-enhancing
    uint16_t bc = (bzVal < -10) ? 0xF800 : (bzVal < -5) ? 0xFFE0 :
                  (bzVal < 0)   ? 0x07FF : 0x07E0;
    gfx->setTextColor(bc);
    char buf[48];
    snprintf(buf, sizeof(buf), "Bz: %+.1f nT    Bt: %.1f nT", bzVal, btVal);
    gfx->setCursor(4, y); gfx->print(buf);
    y += 11;

    gfx->setTextColor(0x7BEF);
    gfx->setCursor(4, y);
    if      (bzVal < -10) gfx->print("Bz strongly south - aurora enhanced");
    else if (bzVal < -5)  gfx->print("Bz southward - favorable for aurora");
    else if (bzVal < 0)   gfx->print("Bz slightly south - mild enhancement");
    else if (bzVal < 5)   gfx->print("Bz near zero - mixed conditions");
    else                  gfx->print("Bz northward - reduced aurora");
    y += 11;
  }

  // Divider + scale reference
  gfx->drawFastHLine(0, y, gfx->width(), 0x2104);
  y += 5;
  gfx->setTextColor(0x7BEF);
  gfx->setTextSize(1);
  gfx->setCursor(4, y);
  gfx->print("G1=Kp5  G2=Kp6  G3=Kp7  G4=Kp8  G5=Kp9");

  Serial.printf("[SW] Kp=%.2f Speed=%.0f Bz=%.1f Bt=%.1f\n", kpVal, swSpeed, bzVal, btVal);
  return true;
}
