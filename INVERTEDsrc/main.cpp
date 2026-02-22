// WeatherCore - Weather Satellite Image for CYD (Cheap Yellow Display)
// Source: NOAA GOES satellite imagery via cdn.star.nesdis.noaa.gov
// Display: ILI9341 320x240 via HWSPI (CYD standard pinout)
// Setup: First boot opens WeatherCore_Setup AP for configuration.
//        On subsequent boots, hold the BOOT button during startup to re-enter setup.
//        WiFi credentials and camera choice are persisted to NVS flash.

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

#include "HTTPS.h"
#include "JPEG.h"

/*******************************************************************************
 * Display setup - CYD (Cheap Yellow Display) proven working config
 * ILI9341 320x240 landscape via hardware SPI
 ******************************************************************************/
#include <Arduino_GFX_Library.h>
#include "Portal.h"

#define GFX_BL 21  // CYD backlight pin

Arduino_DataBus *bus = new Arduino_HWSPI(
    2  /* DC */,
    15 /* CS */,
    14 /* SCK */,
    13 /* MOSI */,
    12 /* MISO */);

Arduino_GFX *gfx = new Arduino_ILI9341(bus, GFX_NOT_DEFINED /* RST */, 1 /* rotation: landscape 320x240 */);
/*******************************************************************************
 * End of display setup
 ******************************************************************************/

// Print a status line on screen (top bar, overwrites previous)
void showStatus(const char *msg) {
  gfx->fillRect(0, 0, gfx->width(), 20, RGB565_BLACK);
  gfx->setTextColor(RGB565_WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(4, 6);
  gfx->print(msg);
  Serial.println(msg);
}

// Draw UTC time in the bottom-right corner (redrawn after decode and every minute)
void drawTimestamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return; // skip if NTP not yet synced
  char buf[12];
  strftime(buf, sizeof(buf), "%H:%M UTC", &timeinfo);
  // textSize(1) = 6px wide x 8px tall per character
  int tw = strlen(buf) * 6;
  int tx = gfx->width()  - tw - 3;
  int ty = gfx->height() - 10;
  gfx->fillRect(tx - 1, ty - 1, tw + 2, 10, RGB565_BLACK);
  gfx->setTextColor(RGB565_WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(tx, ty);
  gfx->print(buf);
}

// JPEGDEC pixel draw callback - writes decoded MCU blocks to the ILI9341
int JPEGDraw(JPEGDRAW *pDraw)
{
  gfx->draw16bitBeRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
  return 1;
}

void setup() {
  Serial.begin(115200);
  Serial.println("WeatherCore - NOAA GOES Satellite (CYD)");

  // Init display
  if (!gfx->begin()) {
    Serial.println("gfx->begin() failed!");
  }
  gfx->invertDisplay(true); // Fix for CYDs with inverted display hardware
  gfx->fillScreen(RGB565_BLACK);

  // Enable backlight
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);

  // Boot button is GPIO 0 (active LOW)
  pinMode(0, INPUT_PULLUP);

  // Load saved settings from flash
  wcLoadSettings();

  bool showPortal = !wc_has_settings;  // always open portal on first boot

  if (!showPortal) {
    // Settings exist — give a 3-second window to hold BOOT button to re-enter setup
    showStatus("Hold BOOT button to change settings...");
    for (int i = 0; i < 30 && !showPortal; i++) {
      if (digitalRead(0) == LOW) showPortal = true;
      delay(100);
    }
  }

  if (showPortal) {
    wcInitPortal();
    while (!portalDone) {
      wcRunPortal();
      delay(5);
    }
    wcClosePortal();
  }

  // Connect to WiFi using saved credentials
  gfx->fillScreen(RGB565_BLACK);
  WiFi.mode(WIFI_STA);
  WiFi.begin(wc_wifi_ssid, wc_wifi_pass);

  int dots = 0;
  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - wifiStart > 30000) {
      char errMsg[60];
      snprintf(errMsg, sizeof(errMsg), "WiFi failed: \"%s\"", wc_wifi_ssid);
      showStatus(errMsg);
      while (true) delay(1000);
    }
    delay(500);
    char msg[48];
    snprintf(msg, sizeof(msg), "Connecting to WiFi%.*s", (dots % 4) + 1, "....");
    showStatus(msg);
    dots++;
  }
  showStatus("WiFi connected!");
  // Sync UTC time via NTP — no user config needed
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  delay(600);
}

#define UPDATE_INTERVAL    (5 * 60 * 1000)  // NOAA updates every ~5 min
#define CLOCK_INTERVAL     (60 * 1000)       // redraw timestamp every minute
unsigned long last_update    = 0;
unsigned long last_clock     = 0;

void loop() {
  // Brief boot button press (GPIO 0) cycles to the next camera view.
  // The hold-at-startup logic in setup() is entirely separate.
  if (digitalRead(0) == LOW) {
    delay(50); // debounce
    if (digitalRead(0) == LOW) {
      // Wait for release (up to 1 second — longer than that is a deliberate hold)
      unsigned long pressStart = millis();
      while (digitalRead(0) == LOW && millis() - pressStart < 1000) delay(10);

      if (millis() - pressStart < 1000) {
        // Brief press: cycle to next camera
        wc_camera_idx = (wc_camera_idx + 1) % NUM_CAMERAS;
        wcSaveCameraIndex(wc_camera_idx);
        char msg[48];
        snprintf(msg, sizeof(msg), "Camera: %s", CAMERAS[wc_camera_idx].name);
        showStatus(msg);
        last_update = 0; // trigger immediate fetch
      }
    }
  }

  if ((last_update == 0) || ((millis() - last_update) > UPDATE_INTERVAL)) {
    Serial.printf("Heap: %d, PSRAM: %d\n", ESP.getFreeHeap(), ESP.getFreePsram());
    showStatus("Fetching GOES satellite image...");

    https_get_response_buf(CAMERAS[wc_camera_idx].url);

    if (https_response_buf && https_response_len > 0 &&
        jpeg.openRAM(https_response_buf, https_response_len, JPEGDraw)) {
      showStatus("Decoding...");
      jpeg.setPixelType(RGB565_BIG_ENDIAN);
      // Clear the full image area before decode to prevent artifacts from
      // previous images and the NOAA watermark bar at the bottom.
      gfx->fillRect(0, 20, gfx->width(), gfx->height() - 20, RGB565_BLACK);
      jpeg.decode(CAMERAS[wc_camera_idx].x_off, CAMERAS[wc_camera_idx].y_off, 0);
      jpeg.close();
      last_update = millis();
      drawTimestamp(); // show time the image was fetched
    } else {
      char errMsg[60];
      snprintf(errMsg, sizeof(errMsg), "Fetch failed HTTP:%d len:%d",
               https_last_http_code, https_response_len);
      showStatus(errMsg);
      Serial.println(errMsg);
      last_update = millis() - UPDATE_INTERVAL + 60000; // retry in 60s
    }

    if (https_response_buf) {
      free(https_response_buf);
      https_response_buf = nullptr;
    }
  }

  // Redraw timestamp every minute so the clock stays current between image refreshes
  if (last_update != 0 && millis() - last_clock > CLOCK_INTERVAL) {
    drawTimestamp();
    last_clock = millis();
  }

  delay(1000);
}
