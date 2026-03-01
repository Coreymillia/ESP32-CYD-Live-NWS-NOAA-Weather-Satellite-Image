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
- **Touch navigation**: tap left third of screen = previous mode, right third = next mode, middle = toggle km/mi units
- **BOOT button**: short press = next mode, long press (≥1.5 s) = reopen WiFi setup portal
- Blue countdown bar at bottom shows time remaining until next refresh
- WiFi auto-reconnects if the connection drops

---

## Hardware

| Component | Details |
|---|---|
| Board | ESP32 (CYD — ESP32-2432S028R or compatible) |
| Display | ILI9341 TFT, 320×240, hardware SPI |
| Backlight | GPIO 21 |
| Display SPI | DC=2, CS=15, SCK=14, MOSI=13, MISO=12 |
| Touch SPI (VSPI) | CLK=25, MISO=39, MOSI=32, CS=33, IRQ=36 |

---

## Navigation

| Action | Result |
|---|---|
| Tap left third of screen | Previous mode |
| Tap right third of screen | Next mode |
| Tap middle of screen | Toggle km ↔ mi (ISS Tracker units) |
| Short press BOOT button | Next mode |
| Long press BOOT button (≥1.5 s) | Reopen WiFi/settings setup portal |

Unit preference (km or mi) is saved to flash and persists across reboots.

---

## Modes

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
| 8 | NWS Forecast (2 periods) | api.weather.gov | 30 min |
| 9 | NWS Alerts | api.weather.gov | 5 min |
| 10 | NOAA Space Weather | NOAA SWPC | 15 min |
| 11 | ISS Live Tracker | wheretheiss.at | 30 sec |

**Modes 8–9 (NWS)** require a US latitude/longitude entered in the setup portal.  
**Mode 10 (Space Weather)** uses your latitude to check if aurora may be visible at your location.  
**Mode 11 (ISS Tracker)** uses your latitude/longitude to compute elevation angle and the 145.800 MHz radio window. Tap the center of the screen to switch between km and mi.

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
| [XPT2046_Touchscreen](https://github.com/PaulStoffregen/XPT2046_Touchscreen) | paulstoffregen | CYD touchscreen input |

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


