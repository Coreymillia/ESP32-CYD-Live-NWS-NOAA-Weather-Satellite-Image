# WeatherCore — Live Weather, Space Weather & ISS Tracker for CYD

A multi-mode live data display running on the **CYD (Cheap Yellow Display)** ESP32 board. Fetches NOAA GOES satellite imagery, NWS text forecasts, NOAA space weather data, and live ISS position — all with no API key required.

---

## Photos

| GOES Storm System | ISS Live Tracker | NOAA Space Weather |
|:---:|:---:|:---:|
| ![GOES satellite image of a storm system](IMG_20260227_223248.jpg) | ![ISS tracker showing position, distance, elevation and 145.800 MHz radio status](IMG_20260227_223216.jpg) | ![NOAA space weather showing Kp 1.7, solar wind, Bz](IMG_20260227_223401.jpg) |

| NWS Forecast | GOES Nighttime |
|:---:|:---:|
| ![NWS forecast showing Tonight: Partly cloudy, low around 29](IMG_20260224_230154.jpg) | ![GOES-East CONUS GeoColor nighttime satellite image](IMG_20260224_230347.jpg) |

---

## What it does

- Connects to your WiFi on boot via a captive portal setup page
- Fetches the latest **NOAA GOES GeoColor** satellite image and renders it to the ILI9341 display — refreshes every **5 minutes**
- Displays **NWS text forecast** and **NWS active alerts** for your latitude/longitude
- Shows **NOAA SWPC space weather** — live Kp index, G-storm level, solar wind speed, and Bz magnetic field — refreshes every **15 minutes**
- Tracks the **ISS live position** with distance, bearing, elevation angle, and a **145.800 MHz FM radio window indicator** — refreshes every **30 seconds**
- Press the **BOOT button** to cycle through all 12 modes at runtime; selection is saved to flash

---

## Hardware

| Component | Details |
|---|---|
| Board | ESP32 (CYD — ESP32-2432S028R or compatible) |
| Display | ILI9341 TFT, 320×240, hardware SPI |
| Backlight | GPIO 21 |
| Display SPI | DC=2, CS=15, SCK=14, MOSI=13, MISO=12 |

---

## Modes

Press the **BOOT button** briefly at runtime to cycle through all modes. Mode is saved to flash.

| # | Mode | Source | Refresh |
|---|---|---|---|
| 0 | GOES-East CONUS *(default)* | NOAA CDN | 5 min |
| 1 | GOES-West CONUS | NOAA CDN | 5 min |
| 2 | Eastern US | NOAA CDN | 5 min |
| 3 | Gulf of Mexico | NOAA CDN | 5 min |
| 4 | Caribbean | NOAA CDN | 5 min |
| 5 | Alaska | NOAA CDN | 5 min |
| 6 | Full Earth Disk | NOAA CDN | 5 min |
| 7 | Mesoscale (hi-refresh) | NOAA CDN | 5 min |
| 8 | NWS Forecast (text) | api.weather.gov | 30 min |
| 9 | NWS Alerts | api.weather.gov | 5 min |
| 10 | NOAA Space Weather | NOAA SWPC | 15 min |
| 11 | ISS Live Tracker | wheretheiss.at | 30 sec |

**Modes 8–9 (NWS)** require a US latitude/longitude entered in the setup portal.  
**Mode 10 (Space Weather)** uses your latitude to check if aurora may be visible at your location.  
**Mode 11 (ISS Tracker)** uses your latitude/longitude to compute elevation angle and the 145.800 MHz radio window.

---

## ISS Tracker — 145.800 MHz Radio Window

The ISS mode calculates the **elevation angle** of the station above your horizon using the haversine surface distance and the standard satellite elevation formula. When elevation is positive the ISS is above your horizon and its FM voice downlink is receivable:

- **Elevation > 10°** — `RADIO ACTIVE` (green) · Strong signal
- **Elevation 0–10°** — `WEAK SIGNAL` (yellow) · Marginal copy
- **Elevation < 0°** — `Offline` · Shows whether ISS is approaching or receding
- Hint: *Passes approximately every 90 minutes. Best window: 5–10 min above 10°*

The display also shows current distance (line-of-sight in km), compass bearing, orbital velocity, and the VISIBLE / DAYLIGHT / ECLIPSED visibility state.

---

## Space Weather — NOAA SWPC

Pulls three live JSON feeds from NOAA's Space Weather Prediction Center (no key required):

| Feed | Data shown |
|---|---|
| Planetary Kp index | Kp value · G-storm level (G0–G5) · color-coded severity |
| Solar wind plasma | Speed in km/s · color-coded (green/yellow/red) |
| Solar wind magnetic field | Bz and Bt in nT · aurora-impact label |

Aurora visibility is estimated from Kp using the empirical formula `77° − (Kp × 3.5°)`. If your saved latitude is above the estimated equatorward boundary, the screen highlights it in green.

---

## Setup

1. **Clone or download** this project and open it in VS Code with PlatformIO installed.

2. **Build and upload:**
   ```
   pio run --target upload
   ```

3. **On first boot the device enters setup mode:**
   - An open WiFi access point called **`WeatherCore_Setup`** appears
   - Connect your phone or PC to that network and open **`192.168.4.1`**
   - Enter your WiFi credentials, choose a starting mode, and enter your **latitude and longitude** (required for NWS, Space Weather aurora check, and ISS tracking)
   - Tap **Save & Connect** — the device connects and the display comes live

4. **On subsequent boots** the device skips the portal and connects automatically.  
   To reconfigure, **hold the BOOT button** within the first 3 seconds after power-on.

> ⚠️ ESP32 only supports **2.4 GHz** WiFi.  
> 💾 All settings are saved to flash (NVS) and survive power cycles.

---

## Project Structure

```
WeatherCore/
├── platformio.ini
├── src/
│   └── main.cpp           — WiFi init, portal, fetch loop, mode dispatch
├── include/
│   ├── Portal.h           — Captive portal, web UI, NVS settings persistence
│   ├── HTTPS.h            — WiFiClientSecure HTTPS GET with chunked transfer
│   ├── JPEG.h             — JPEGDEC instance and decode callback
│   ├── NWSForecast.h      — NWS forecast + alerts fetch and display
│   ├── SpaceWeather.h     — NOAA SWPC Kp, solar wind, Bz fetch and display
│   └── ISSTracker.h       — ISS live position, elevation, radio window
└── INVERTEDWeatherCore/   — Identical build for CYDs with inverted display hardware
    ├── platformio.ini
    ├── src/main.cpp        — Same + gfx->invertDisplay(true)
    └── include/            — Identical headers
```

---

## Inverted Display Variant

Some CYD boards ship with an inverted display — white background, dark text. If your screen looks wrong, flash the **`INVERTEDWeatherCore`** project instead. It is byte-for-byte identical except for one added line in `setup()`:

```cpp
gfx->invertDisplay(true);
```

All 12 modes work identically on the inverted variant.

---

## Dependencies

Managed automatically by PlatformIO:

| Library | Author | Purpose |
|---|---|---|
| [GFX Library for Arduino](https://github.com/moononournation/Arduino_GFX) @ 1.4.7 | moononournation | ILI9341 display driver |
| [JPEGDEC](https://github.com/bitbank2/JPEGDEC) | bitbank2 | In-memory JPEG decoding |
| [ArduinoJson](https://arduinojson.org/) @^6 | bblanchon | JSON parsing (NWS, SWPC, ISS) |
| [XPT2046_Touchscreen](https://github.com/PaulStoffregen/XPT2046_Touchscreen) | paulstoffregen | CYD touch (boot button detection) |

---

## Data Sources

All feeds are free and require no API key:

| Data | URL |
|---|---|
| GOES satellite images | `cdn.star.nesdis.noaa.gov` |
| NWS forecast + alerts | `api.weather.gov` |
| NOAA Kp index | `services.swpc.noaa.gov/products/noaa-planetary-k-index.json` |
| Solar wind plasma | `services.swpc.noaa.gov/products/solar-wind/plasma-5-minute.json` |
| Solar wind mag field | `services.swpc.noaa.gov/products/solar-wind/mag-5-minute.json` |
| ISS position | `api.wheretheiss.at/v1/satellites/25544` |

---

## Credits & Acknowledgements

### [WeatherPanel](https://github.com/moononournation/WeatherPanel) by moononournation
The original **WeatherSatelliteImage** Arduino sketch that inspired this project. The core HTTPS fetch logic (`HTTPS.h`) and JPEG decode callback pattern come directly from this project, which targets a different ESP32 board (AXS15231B QSPI 172×640) and fetches FY-4B satellite imagery from China's NMC weather service.

---

## License

This project builds on open-source work. Please respect the licenses of the upstream projects listed above.


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
- Displays the **NWS text forecast** and **NWS active alerts** for your location
- Press the **BOOT button** at runtime to cycle through all satellite views and NWS modes
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
│   └── main.cpp           — WiFi, portal init, fetch loop, JPEG decode, display
├── include/
│   ├── Portal.h           — Captive portal: AP setup, web UI, NVS settings persistence
│   ├── NWSForecast.h      — NWS API fetch, JSON parse, forecast + alerts display
│   ├── HTTPS.h            — WiFiClientSecure HTTPS GET with chunked transfer support
│   └── JPEG.h             — JPEGDEC instance
│
│   ── Inverted display variant (see below) ──
├── INVERTEDsrc/
│   └── main.cpp           — Same as above with invertDisplay(true)
├── INVERTEDinclude/
│   ├── Portal.h
│   ├── NWSForecast.h      — Colors adjusted for white-background inverted display
│   ├── HTTPS.h
│   └── JPEG.h
└── INVERTEDplatformio/
    └── platformio.ini
```

---

## Setup

1. **Clone or download** this project and open it in VS Code with PlatformIO installed.

2. **Build and upload:**
   ```
   pio run --target upload
   ```

3. **On first boot the device enters setup mode:**
   - The display shows step-by-step instructions
   - An open WiFi access point called **`WeatherCore_Setup`** is created
   - Connect your phone or PC to that network, then open **`192.168.4.1`** in your browser
   - Enter your WiFi credentials, choose a NOAA satellite view, and optionally enter your **latitude and longitude** for NWS forecast and alerts
   - Tap **Save & Connect**
   - The portal closes, the device connects to your WiFi, and the satellite image appears

4. **On subsequent boots** the device skips the portal and connects automatically using saved settings.  
   To change your WiFi or settings, **hold the BOOT button** during the first 3 seconds after power-on — the setup portal will reopen.

5. **To switch modes at runtime**, press the **BOOT button** briefly to cycle through all 10 modes. The selected mode is saved to flash.

> ⚠️ ESP32 only supports **2.4 GHz** WiFi networks.

> 💾 WiFi credentials and mode selection are saved to flash (NVS) and survive power cycles.

---

## Inverted Display Variant

Some CYD boards ship with an inverted display — everything appears with a **white background and dark text/colors** instead of black. If your screen looks inverted, use the files from the `INVERTED*` folders instead:

1. Replace the contents of `src/` with the files from `INVERTEDsrc/`
2. Replace the contents of `include/` with the files from `INVERTEDinclude/`
3. Replace `platformio.ini` with the one from `INVERTEDplatformio/`
4. Build and upload as normal

The inverted variant is identical in functionality — all 10 modes work the same way. Colors are adjusted so the display looks correct on white-background hardware.

---

## Modes

Press the **BOOT button** briefly at runtime to cycle through all modes. Mode is saved to flash.

| # | Mode | Refresh |
|---|---|---|
| 0 | GOES-East CONUS *(default)* | 5 min |
| 1 | GOES-West CONUS | 5 min |
| 2 | Eastern US | 5 min |
| 3 | Gulf of Mexico | 5 min |
| 4 | Caribbean | 5 min |
| 5 | Alaska | 5 min |
| 6 | Full Earth Disk | 5 min |
| 7 | Mesoscale (hi-refresh) | 5 min |
| 8 | NWS Forecast (Text) | 30 min |
| 9 | NWS Alerts | 5 min |

**NWS modes require a US location.** Enter your latitude and longitude in the setup portal (e.g. `38.71`, `-105.14`). No API key required — powered by [api.weather.gov](https://www.weather.gov/documentation/services-web-api).

NWS Alerts shows active warnings and watches for your area, or a green "No active alerts" confirmation when the area is clear.

To reconfigure WiFi or coordinates, reboot and hold BOOT within 3 seconds to reopen the portal.

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

## Credits & Acknowledgements

### [WeatherPanel](https://github.com/moononournation/WeatherPanel) by moononournation
The original **WeatherSatelliteImage** Arduino sketch that inspired this project. It targets a different ESP32 board (AXS15231B QSPI 172×640 display) and fetches FY-4B satellite imagery from China's NMC weather service. The core HTTPS fetch logic (`HTTPS.h`) and JPEG decode callback pattern come directly from this project.

> Original designed for the FengYun FY-4B geostationary satellite over Asia. Adapted here for NOAA GOES and the CYD hardware platform.

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

