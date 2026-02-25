# WeatherCore — NOAA GOES Satellite Image + NWS Forecast for CYD

A live NOAA GOES satellite image viewer and NWS text forecast display running on the **CYD (Cheap Yellow Display)** ESP32 board. Fetches and displays a fresh satellite image every 5 minutes and a local NWS forecast every 30 minutes — no API key required.

| NWS Forecast | GOES Satellite (nighttime) |
|:---:|:---:|
| ![NWS forecast showing Tonight: Partly cloudy, low around 29](IMG_20260224_230154.jpg) | ![GOES-East CONUS GeoColor nighttime satellite image](IMG_20260224_230347.jpg) |

---

## What it does

- Connects to your WiFi network on boot
- Fetches the latest **NOAA GOES GeoColor** satellite image from `cdn.star.nesdis.noaa.gov`
- Decodes the JPEG in memory and renders it to the ILI9341 display
- Refreshes every **5 minutes** (NOAA updates the image at that frequency)
- Optionally displays the **NWS text forecast** for your location, refreshed every **30 minutes**
- Press the **BOOT button** at runtime to cycle through satellite views and NWS forecast mode
- Shows on-screen status messages throughout (WiFi connecting, fetching, decoding)

---

## Hardware

| Component | Details |
|---|---|
| Board | ESP32 (CYD — ESP32-2432S028R or compatible) |
| Display | ILI9341 TFT, 320×240, hardware SPI |
| Backlight | GPIO 21 |
| Display SPI | DC=2, CS=15, SCK=14, MOSI=13, MISO=12 |

---

## Project Structure

```
WeatherCore/
├── platformio.ini
├── src/
│   └── main.cpp          — WiFi, portal init, fetch loop, JPEG decode, display
└── include/
    ├── Portal.h          — Captive portal: AP setup, web UI, NVS settings persistence
    ├── NWSForecast.h     — NWS API fetch, JSON parse, forecast display
    ├── HTTPS.h           — WiFiClientSecure HTTPS GET with chunked transfer support
    └── JPEG.h            — JPEGDEC instance
```

---

## Setup

1. **Clone or download** this project and open it in VS Code with PlatformIO installed.

2. **Build and upload:**
   ```
   pio run --target upload
   ```
   If you get a White background then use the INVERTED folders files. 

4. **On first boot the device enters setup mode:**
   - The display shows step-by-step instructions
   - An open WiFi access point called **`WeatherCore_Setup`** is created
   - Connect your phone or PC to that network, then IF CAPTIVE PORTAL DOES NOT OPEN, THEN open **`192.168.4.1`** in your browser
   - Enter your WiFi credentials, choose a NOAA satellite view, and optionally enter your **latitude and longitude** for NWS forecast mode
   - Tap **Save & Connect**
   - The portal closes, the device connects to your WiFi, and the satellite image appears

5. **On subsequent boots** the device skips the portal and connects automatically using saved settings.  
   To change your WiFi or camera, **hold the BOOT button** during the first 3 seconds after power-on — the setup portal will reopen.

6. **To switch modes at runtime**, press the **BOOT button** briefly to cycle through all satellite views and NWS Forecast mode. The selected mode is saved to flash.

---

## NWS Forecast Mode

When **NWS Forecast (Text)** is selected, the device fetches the current forecast period for your location from the [National Weather Service API](https://www.weather.gov/documentation/services-web-api) (`api.weather.gov`) — no API key required.

- Enter your **latitude and longitude** in the setup portal (e.g. `38.71`, `-105.14` for Victor, CO)
- The forecast period name (e.g. *Tonight*) is shown in cyan; the detailed forecast follows in white
- Refreshes every **30 minutes**
- US locations only (NWS coverage)

> ⚠️ ESP32 only supports **2.4 GHz** WiFi networks.

> 💾 WiFi credentials and camera choice are saved to flash (NVS) and survive power cycles.

---

## Dependencies

Managed automatically by PlatformIO:

| Library | Author | Purpose |
|---|---|---|
| [GFX Library for Arduino](https://github.com/moononournation/Arduino_GFX) @ 1.4.7 | moononournation | ILI9341 display driver |
| [JPEGDEC](https://github.com/bitbank2/JPEGDEC) | bitbank2 | In-memory JPEG decoding |
| [ArduinoJson](https://arduinojson.org/) | bblanchon | NWS API JSON parsing |
| [XPT2046_Touchscreen](https://github.com/PaulStoffregen/XPT2046_Touchscreen) | paulstoffregen | CYD touch (included for compatibility) |

---

## Image Source

**NOAA GOES satellite imagery** — free, public, no authentication required, updated every ~5 minutes.

The satellite view is selected at runtime via the captive portal. Available options:

| Option | Satellite | Coverage |
|---|---|---|
| **GOES-East CONUS** *(default)* | GOES-16 | Full continental US, eastern focus |
| **GOES-West CONUS** | GOES-18 | Full continental US, western focus |
| **Eastern US** | GOES-19 | Eastern seaboard + Appalachians |
| **Gulf of Mexico** | GOES-19 | Gulf Coast, Mexico, Central America |
| **Caribbean** | GOES-19 | Gulf of Mexico + Caribbean Sea |
| **Alaska** | GOES-18 | Alaska region |

To change your view or switch to NWS forecast mode, press the **BOOT button** briefly at runtime to cycle through all options. To reconfigure WiFi or coordinates, reboot and hold BOOT within 3 seconds to reopen the portal.

---

## Credits & Acknowledgements

This project is a port and adaptation of the following open-source works:

### [WeatherPanel](https://github.com/moononournation/WeatherPanel) by moononournation
The original **WeatherSatelliteImage** Arduino sketch that inspired this project. It targets a different ESP32 board (AXS15231B QSPI 172×640 display) and fetches FY-4B satellite imagery from China's NMC weather service. The core HTTPS fetch logic (`HTTPS.h`) and JPEG decode callback pattern come directly from this project.

> Original designed for the FengYun FY-4B geostationary satellite over Asia. Adapted here for NOAA GOES-East and the CYD hardware platform.

---

## Conversion Notes

Key changes made when porting WeatherSatelliteImage → WeatherCore:

| Original | WeatherCore |
|---|---|
| AXS15231B QSPI 172×640 display | ILI9341 HWSPI 320×240 (CYD) |
| `Arduino_Canvas` wrapper | Direct rendering |
| `NetworkClientSecure` (Arduino ESP32 3.x) | `WiFiClientSecure` (Arduino ESP32 2.x) |
| FY-4B NMC image server (Asia-only) | NOAA GOES CDN (worldwide) |
| Timestamped URL construction | Static "latest image" URL |
| NTP required for URL building | NTP not needed |

---

## License

This project builds on open-source work. Please respect the licenses of the upstream projects listed above.
