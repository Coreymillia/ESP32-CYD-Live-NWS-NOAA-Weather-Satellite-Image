#pragma once

#include <Arduino_GFX_Library.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

// gfx is defined in main.cpp
extern Arduino_GFX *gfx;

// ---------------------------------------------------------------------------
// Camera definitions
// ---------------------------------------------------------------------------
struct CameraOption {
  const char *name;
  const char *url;
};

static const CameraOption CAMERAS[] = {
  { "GOES-East CONUS (default)",  "https://cdn.star.nesdis.noaa.gov/GOES16/ABI/CONUS/GEOCOLOR/416x250.jpg" },
  { "GOES-West CONUS",            "https://cdn.star.nesdis.noaa.gov/GOES18/ABI/CONUS/GEOCOLOR/416x250.jpg" },
  { "GOES-East Caribbean",        "https://cdn.star.nesdis.noaa.gov/GOES16/ABI/SECTOR/car/GEOCOLOR/416x250.jpg" },
  { "GOES-East Alaska",           "https://cdn.star.nesdis.noaa.gov/GOES18/ABI/SECTOR/ak/GEOCOLOR/416x250.jpg" },
  { "GOES-West Hawaii",           "https://cdn.star.nesdis.noaa.gov/GOES18/ABI/SECTOR/hi/GEOCOLOR/416x250.jpg" },
};
static const int NUM_CAMERAS = 5;

// ---------------------------------------------------------------------------
// Persisted settings (populated by wcInitPortal / wcLoadSettings)
// ---------------------------------------------------------------------------
static char wc_wifi_ssid[64] = "";
static char wc_wifi_pass[64] = "";
static int  wc_camera_idx    = 0;
static bool wc_has_settings  = false;  // true if SSID was previously saved

// ---------------------------------------------------------------------------
// Portal state
// ---------------------------------------------------------------------------
static WebServer  *portalServer = nullptr;
static DNSServer  *portalDNS    = nullptr;
static bool        portalDone   = false;

// ---------------------------------------------------------------------------
// NVS helpers
// ---------------------------------------------------------------------------
static void wcLoadSettings() {
  Preferences prefs;
  prefs.begin("weathercore", true);
  String ssid = prefs.getString("ssid", "");
  String pass = prefs.getString("pass", "");
  wc_camera_idx = prefs.getInt("camera", 0);
  prefs.end();

  wc_camera_idx = constrain(wc_camera_idx, 0, NUM_CAMERAS - 1);
  ssid.toCharArray(wc_wifi_ssid, sizeof(wc_wifi_ssid));
  pass.toCharArray(wc_wifi_pass, sizeof(wc_wifi_pass));
  wc_has_settings = (ssid.length() > 0);
}

static void wcSaveSettings(const char *ssid, const char *pass, int camera) {
  Preferences prefs;
  prefs.begin("weathercore", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.putInt("camera", camera);
  prefs.end();

  strncpy(wc_wifi_ssid, ssid, sizeof(wc_wifi_ssid) - 1);
  strncpy(wc_wifi_pass, pass, sizeof(wc_wifi_pass) - 1);
  wc_camera_idx   = camera;
  wc_has_settings = true;
}

// ---------------------------------------------------------------------------
// On-screen setup instructions (320x240 landscape)
// ---------------------------------------------------------------------------
static void wcShowPortalScreen() {
  gfx->fillScreen(RGB565_BLACK);

  // Title
  gfx->setTextColor(0x07FF);  // cyan
  gfx->setTextSize(2);
  gfx->setCursor(22, 5);
  gfx->print("WeatherCore Setup");

  // Divider hint
  gfx->setTextColor(RGB565_WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(50, 26);
  gfx->print("Satellite Image Viewer");

  // Step 1
  gfx->setTextColor(0xFFE0);  // yellow
  gfx->setCursor(4, 46);
  gfx->print("1. Connect your phone/PC to WiFi:");
  gfx->setTextColor(0x07FF);  // cyan
  gfx->setTextSize(2);
  gfx->setCursor(14, 58);
  gfx->print("WeatherCore_Setup");

  // Step 2
  gfx->setTextColor(0xFFE0);
  gfx->setTextSize(1);
  gfx->setCursor(4, 82);
  gfx->print("2. Open your browser and go to:");
  gfx->setTextColor(0x07FF);
  gfx->setTextSize(2);
  gfx->setCursor(50, 94);
  gfx->print("192.168.4.1");

  // Step 3
  gfx->setTextColor(0xFFE0);
  gfx->setTextSize(1);
  gfx->setCursor(4, 118);
  gfx->print("3. Enter your WiFi & camera, then");
  gfx->setCursor(4, 130);
  gfx->print("   tap  Save & Connect.");

  // Note if settings already exist
  if (wc_has_settings) {
    gfx->setTextColor(0x07E0);  // green
    gfx->setCursor(4, 152);
    gfx->print("Existing settings found. Tap");
    gfx->setCursor(4, 164);
    gfx->print("'No Changes' to keep them.");
  }
}

// ---------------------------------------------------------------------------
// Web handlers
// ---------------------------------------------------------------------------
static void wcHandleRoot() {
  String html = "<!DOCTYPE html><html><head>"
    "<meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>WeatherCore Setup</title>"
    "<style>"
    "body{background:#001a33;color:#00ccff;font-family:Arial,sans-serif;"
         "text-align:center;padding:20px;max-width:480px;margin:auto;}"
    "h1{color:#00ffff;font-size:1.6em;margin-bottom:4px;}"
    "p{color:#88aacc;font-size:0.9em;}"
    "label{display:block;text-align:left;margin:14px 0 4px;color:#88ddff;font-weight:bold;}"
    "input,select{width:100%;box-sizing:border-box;background:#002244;color:#00ccff;"
                 "border:2px solid #0066aa;border-radius:6px;padding:10px;font-size:1em;}"
    ".btn{display:block;width:100%;padding:14px;margin:10px 0;font-size:1.05em;"
         "border-radius:8px;border:none;cursor:pointer;font-weight:bold;}"
    ".btn-save{background:#004488;color:#00ffff;border:2px solid #0099dd;}"
    ".btn-save:hover{background:#0066bb;}"
    ".btn-skip{background:#1a1a2e;color:#667788;border:2px solid #334455;}"
    ".btn-skip:hover{background:#223344;color:#aabbcc;}"
    ".note{color:#445566;font-size:0.82em;margin-top:16px;}"
    "hr{border:1px solid #113355;margin:20px 0;}"
    "</style></head><body>"
    "<h1>&#127780; WeatherCore Setup</h1>"
    "<p>Enter your WiFi and choose a NOAA satellite view.</p>"
    "<form method='post' action='/save'>"
    "<label>WiFi Network Name (SSID):</label>"
    "<input type='text' name='ssid' value='";
  html += String(wc_wifi_ssid);
  html += "' placeholder='Your 2.4 GHz WiFi name' maxlength='63' required>"
    "<label>WiFi Password:</label>"
    "<input type='password' name='pass' value='";
  html += String(wc_wifi_pass);
  html += "' placeholder='Leave blank if open network' maxlength='63'>"
    "<label>NOAA Satellite View:</label>"
    "<select name='camera'>";
  for (int i = 0; i < NUM_CAMERAS; i++) {
    html += "<option value='" + String(i) + "'";
    if (i == wc_camera_idx) html += " selected";
    html += ">" + String(CAMERAS[i].name) + "</option>";
  }
  html += "</select>"
    "<br><button class='btn btn-save' type='submit'>&#128190; Save &amp; Connect</button>"
    "</form>";
  if (wc_has_settings) {
    html += "<hr>"
      "<form method='post' action='/nochange'>"
      "<button class='btn btn-skip' type='submit'>&#10006; No Changes &mdash; Use Current Settings</button>"
      "</form>";
  }
  html += "<p class='note'>&#9888; ESP32 supports 2.4 GHz WiFi networks only.</p>"
    "</body></html>";

  portalServer->send(200, "text/html", html);
}

static void wcHandleSave() {
  String ssid   = portalServer->hasArg("ssid")   ? portalServer->arg("ssid")            : "";
  String pass   = portalServer->hasArg("pass")   ? portalServer->arg("pass")            : "";
  int    camera = portalServer->hasArg("camera") ? portalServer->arg("camera").toInt()  : 0;
  camera = constrain(camera, 0, NUM_CAMERAS - 1);

  if (ssid.length() == 0) {
    portalServer->send(400, "text/html",
      "<html><body style='background:#001a33;color:#ff5555;font-family:Arial;"
      "text-align:center;padding:40px'>"
      "<h2>&#10060; SSID cannot be empty!</h2>"
      "<a href='/' style='color:#00ccff'>&#8592; Go Back</a></body></html>");
    return;
  }

  wcSaveSettings(ssid.c_str(), pass.c_str(), camera);

  String html = "<html><head><meta charset='UTF-8'>"
    "<style>body{background:#001a33;color:#00ccff;font-family:Arial;"
    "text-align:center;padding:40px;}h2{color:#00ffff;}"
    "p{color:#88aacc;}</style></head><body>"
    "<h2>&#9989; Settings Saved!</h2>"
    "<p>Connecting to <b>" + ssid + "</b>...</p>"
    "<p>Satellite view: <b>" + String(CAMERAS[camera].name) + "</b></p>"
    "<p>You can close this page and disconnect from <b>WeatherCore_Setup</b>.</p>"
    "<p style='color:#445566;font-size:0.85em'>The display will show the satellite image shortly.</p>"
    "</body></html>";
  portalServer->send(200, "text/html", html);

  delay(1500);
  portalDone = true;
}

static void wcHandleNoChange() {
  String html = "<html><head><meta charset='UTF-8'>"
    "<style>body{background:#001a33;color:#00ccff;font-family:Arial;"
    "text-align:center;padding:40px;}h2{color:#00ffff;}"
    "p{color:#88aacc;}</style></head><body>"
    "<h2>&#128077; No Changes</h2>"
    "<p>Using your saved settings. Device is connecting now.</p>"
    "<p>You can close this page and disconnect from <b>WeatherCore_Setup</b>.</p>"
    "</body></html>";
  portalServer->send(200, "text/html", html);

  delay(1500);
  portalDone = true;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

// Call once in setup() — loads settings, starts AP, shows instructions
static void wcInitPortal() {
  wcLoadSettings();

  WiFi.mode(WIFI_AP);
  WiFi.softAP("WeatherCore_Setup", "");
  delay(500);

  portalDNS    = new DNSServer();
  portalServer = new WebServer(80);

  portalDNS->start(53, "*", WiFi.softAPIP());

  portalServer->on("/",         wcHandleRoot);
  portalServer->on("/save",     HTTP_POST, wcHandleSave);
  portalServer->on("/nochange", HTTP_POST, wcHandleNoChange);
  portalServer->onNotFound(wcHandleRoot);
  portalServer->begin();

  portalDone = false;

  wcShowPortalScreen();

  Serial.printf("[Portal] AP up — connect to WeatherCore_Setup, open %s\n",
                WiFi.softAPIP().toString().c_str());
}

// Call in a tight loop until portalDone is true
static void wcRunPortal() {
  portalDNS->processNextRequest();
  portalServer->handleClient();
}

// Call after portalDone — tears down AP and frees resources
static void wcClosePortal() {
  portalServer->stop();
  portalDNS->stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(300);

  delete portalServer; portalServer = nullptr;
  delete portalDNS;    portalDNS    = nullptr;
}
