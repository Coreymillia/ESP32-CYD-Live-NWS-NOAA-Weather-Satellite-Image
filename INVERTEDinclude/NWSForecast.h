#pragma once

#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Arduino_GFX_Library.h>

#define NWS_USER_AGENT      "esp32-cyd-weather (github.com/Coreymillia)"
#define NWS_UPDATE_INTERVAL (30UL * 60UL * 1000UL)  // 30 minutes
#define NWS_ALERTS_INTERVAL  ( 5UL * 60UL * 1000UL)  //  5 minutes

// gfx is defined in main.cpp
extern Arduino_GFX *gfx;

// Fetch a URL with the NWS-required User-Agent and return the body as a String.
static String nws_https_get(const String &url) {
  Serial.printf("[NWS] GET %s\n", url.c_str());
  WiFiClientSecure *client = new WiFiClientSecure;
  if (!client) return "";
  client->setInsecure();
  String body;
  {
    HTTPClient https;
    https.begin(*client, url);
    https.addHeader("User-Agent", NWS_USER_AGENT);
    https.addHeader("Accept", "application/geo+json");
    https.setTimeout(15000);
    https.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    int code = https.GET();
    if (code == HTTP_CODE_OK) {
      body = https.getString();
    } else {
      Serial.printf("[NWS] HTTP error: %d\n", code);
    }
    https.end();
  }
  delete client;
  return body;
}

// Word-wrap and draw text on the display. Returns the y position after the last line.
// Note: with invertDisplay(true), RGB565_WHITE draws as black on the white background.
static int nws_draw_wrapped(const String &text, int x, int y, int maxW, uint16_t color) {
  gfx->setTextColor(color);
  gfx->setTextSize(1);
  const int charW = 6, lineH = 10;
  int charsPerLine = maxW / charW;
  int pos = 0, len = text.length();
  while (pos < len && y < 228) {
    int end = pos + charsPerLine;
    if (end >= len) {
      end = len;
    } else {
      // Back up to last space so we don't break mid-word
      for (int i = end; i > pos; i--) {
        if (text[i] == ' ') { end = i; break; }
      }
    }
    String line = text.substring(pos, end);
    line.trim();
    gfx->setCursor(x, y);
    gfx->print(line);
    y += lineH;
    pos = end;
    while (pos < len && text[pos] == ' ') pos++;
  }
  return y;
}

// Fetch the NWS forecast for the given lat/lon and draw it on screen.
// Returns true on success, false on any failure.
// Color scheme for inverted display (invertDisplay(true)):
//   RGB565_BLACK fill  → appears white  (background)
//   0xF800 (red) text  → appears cyan   (period name accent)
//   RGB565_WHITE text  → appears black  (forecast body)
bool nwsFetchAndDisplay(const char *lat, const char *lon) {
  // ── Step 1: /points → resolve the forecast URL for this location ──────────
  String pointsUrl = String("https://api.weather.gov/points/") + lat + "," + lon;
  String pointsBody = nws_https_get(pointsUrl);
  if (pointsBody.isEmpty()) return false;

  DynamicJsonDocument pointsDoc(4096);
  if (deserializeJson(pointsDoc, pointsBody)) {
    Serial.println("[NWS] Points JSON parse failed");
    return false;
  }
  String forecastUrl = pointsDoc["properties"]["forecast"] | "";
  pointsDoc.clear();

  if (forecastUrl.isEmpty()) {
    Serial.println("[NWS] No forecast URL in points response");
    return false;
  }
  Serial.printf("[NWS] Forecast URL: %s\n", forecastUrl.c_str());

  // ── Step 2: forecast → extract first period ───────────────────────────────
  String forecastBody = nws_https_get(forecastUrl);
  if (forecastBody.isEmpty()) return false;

  // Filter to only parse the fields we need (reduces JsonDocument size)
  StaticJsonDocument<128> filter;
  filter["properties"]["periods"][0]["name"] = true;
  filter["properties"]["periods"][0]["detailedForecast"] = true;

  DynamicJsonDocument forecastDoc(8192);
  if (deserializeJson(forecastDoc, forecastBody, DeserializationOption::Filter(filter))) {
    Serial.println("[NWS] Forecast JSON parse failed");
    return false;
  }

  String periodName       = forecastDoc["properties"]["periods"][0]["name"]            | "Unknown";
  String detailedForecast = forecastDoc["properties"]["periods"][0]["detailedForecast"] | "No forecast available.";
  forecastDoc.clear();

  Serial.printf("[NWS] %s: %s\n", periodName.c_str(), detailedForecast.c_str());

  // ── Draw on screen ────────────────────────────────────────────────────────
  // RGB565_BLACK fill → displays as white on inverted hardware
  gfx->fillRect(0, 20, gfx->width(), gfx->height() - 20, RGB565_BLACK);

  // Period name: draw 0xF800 (red) → displays as cyan accent on white background
  gfx->setTextColor(0xF800);
  gfx->setTextSize(2);
  gfx->setCursor(4, 25);
  gfx->print(periodName);

  // Detailed forecast: draw RGB565_WHITE → displays as black on white background
  nws_draw_wrapped(detailedForecast, 4, 45, gfx->width() - 8, RGB565_WHITE);

  return true;
}

// ── NWS Active Alerts ─────────────────────────────────────────────────────────
// Color scheme for inverted display:
//   RGB565_BLACK fill  → appears white  (background)
//   0x07E0 (green) text → appears magenta/red (all-clear header)
//   0x001F (blue) text  → appears red/orange  (alert header)
//   0xFFE0 (yellow) text → appears blue       (event name)
//   RGB565_WHITE text  → appears black        (body)
bool nwsFetchAndDisplayAlerts(const char *lat, const char *lon) {
  String url = String("https://api.weather.gov/alerts/active?point=") + lat + "," + lon;
  String body = nws_https_get(url);
  if (body.isEmpty()) return false;

  StaticJsonDocument<128> filter;
  filter["features"][0]["properties"]["event"]    = true;
  filter["features"][0]["properties"]["headline"] = true;

  DynamicJsonDocument doc(3072);
  if (deserializeJson(doc, body, DeserializationOption::Filter(filter))) {
    Serial.println("[NWS] Alerts JSON parse failed");
    return false;
  }

  JsonArray features = doc["features"];
  int alertCount = features.size();

  gfx->fillRect(0, 20, gfx->width(), gfx->height() - 20, RGB565_BLACK);

  if (alertCount == 0) {
    // 0x07E0 (green) → displays as magenta on inverted, use 0x001F (blue) → red/orange
    gfx->setTextColor(0x07E0);
    gfx->setTextSize(2);
    gfx->setCursor(4, 30);
    gfx->print("NWS Alerts");
    gfx->setTextColor(RGB565_WHITE);
    gfx->setTextSize(1);
    gfx->setCursor(4, 58);
    gfx->print("No active alerts");
    gfx->setCursor(4, 70);
    gfx->print("for your area.");
  } else {
    // Alert count header: 0xF800 (red) → displays as cyan on inverted
    gfx->setTextColor(0xF800);
    gfx->setTextSize(2);
    gfx->setCursor(4, 25);
    char title[24];
    snprintf(title, sizeof(title), "%d Alert%s!", alertCount, alertCount > 1 ? "s" : "");
    gfx->print(title);

    int y = 46;
    int shown = 0;
    for (JsonObject feature : features) {
      if (shown >= 2) break;
      String event    = feature["properties"]["event"]    | "Unknown Event";
      String headline = feature["properties"]["headline"] | "";

      // 0xFFE0 (yellow) → displays as blue on inverted (accent for event name)
      gfx->setTextColor(0xFFE0);
      gfx->setTextSize(1);
      gfx->setCursor(4, y);
      if (event.length() > 35) event = event.substring(0, 34);
      gfx->print(event);
      y += 12;

      if (headline.length() > 120) headline = headline.substring(0, 119);
      y = nws_draw_wrapped(headline, 4, y, gfx->width() - 8, RGB565_WHITE);
      y += 4;
      shown++;
    }
  }

  Serial.printf("[NWS] Alerts: %d active\n", alertCount);
  return true;
}
