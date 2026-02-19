// WeatherCore - Weather Satellite Image for CYD (Cheap Yellow Display)
// Source: NOAA GOES-East CONUS (Americas) via cdn.star.nesdis.noaa.gov
// Display: ILI9341 320x240 via HWSPI (CYD standard pinout)

const char *SSID_NAME = "your-ssid";
const char *SSID_PASSWORD = "your-password";

// NOAA GOES-East CONUS GeoColor 416x250 - small enough for ESP32 heap (~30-50KB)
// Cropped to fill 320x240: offset (-48, -5)
const char *SATELLITE_URL = "https://cdn.star.nesdis.noaa.gov/GOES16/ABI/CONUS/GEOCOLOR/416x250.jpg";

#include <Arduino.h>
#include <WiFi.h>

char url[256];

#include "HTTPS.h"
#include "JPEG.h"

/*******************************************************************************
 * Display setup - CYD (Cheap Yellow Display) proven working config
 * ILI9341 320x240 landscape via hardware SPI
 ******************************************************************************/
#include <Arduino_GFX_Library.h>

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
  gfx->fillScreen(RGB565_BLACK);

  // Enable backlight
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);

  // Connect to WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID_NAME, SSID_PASSWORD);

  int dots = 0;
  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - wifiStart > 30000) {
      char errMsg[60];
      snprintf(errMsg, sizeof(errMsg), "WiFi failed: \"%s\"", SSID_NAME);
      showStatus(errMsg);
      while (true) delay(1000);
    }
    delay(500);
    char msg[40];
    snprintf(msg, sizeof(msg), "Connecting to WiFi%.*s", (dots % 4) + 1, "....");
    showStatus(msg);
    dots++;
  }
  showStatus("WiFi connected!");
  delay(600);
}

#define UPDATE_INTERVAL (5 * 60 * 1000)  // NOAA updates every ~5 min
unsigned long last_update = 0;

void loop() {
  if ((last_update == 0) || ((millis() - last_update) > UPDATE_INTERVAL)) {
    Serial.printf("Heap: %d, PSRAM: %d\n", ESP.getFreeHeap(), ESP.getFreePsram());
    showStatus("Fetching GOES satellite image...");

    https_get_response_buf(SATELLITE_URL);

    if (https_response_buf && https_response_len > 0 &&
        jpeg.openRAM(https_response_buf, https_response_len, JPEGDraw)) {
      showStatus("Decoding...");
      jpeg.setPixelType(RGB565_BIG_ENDIAN);
      // 416x250 source, no scaling needed. Crop to 320x240: offset (-48,-5).
      jpeg.decode(-48 /* x */, -5 /* y */, 0 /* no scaling */);
      jpeg.close();
      last_update = millis(); // success: wait 5min before next fetch
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

  delay(1000);
}
