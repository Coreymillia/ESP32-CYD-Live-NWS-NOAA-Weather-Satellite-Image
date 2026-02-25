#pragma once

#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Arduino_GFX_Library.h>

#define NWS_USER_AGENT      "esp32-cyd-weather (github.com/Coreymillia)"
#define NWS_UPDATE_INTERVAL (30UL * 60UL * 1000UL)  // 30 minutes

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
