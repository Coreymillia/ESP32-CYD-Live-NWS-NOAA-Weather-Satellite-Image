# WeatherCore — NOAA GOES Satellite Image for CYD

A live NOAA GOES-East satellite image viewer running on the **CYD (Cheap Yellow Display)** ESP32 board. Fetches and displays a fresh satellite image every 5 minutes directly from NOAA's public CDN — no API key required.

![CYD displaying NOAA GOES-East CONUS GeoColor satellite image](https://cdn.star.nesdis.noaa.gov/GOES16/ABI/CONUS/GEOCOLOR/416x250.jpg)

---

## What it does

- Connects to your WiFi network on boot (AFTER SETTING WIFI)
- Fetches the latest **NOAA GOES-East CONUS GeoColor** satellite image from `cdn.star.nesdis.noaa.gov`
- Decodes the JPEG in memory and renders it to the ILI9341 display
- Refreshes every **5 minutes** (NOAA updates the image at that frequency)
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
If you get a white screen use the INVERTED FILES!!!

3. **On first boot the device enters setup mode:**
   - The display shows step-by-step instructions
   - An open WiFi access point called **`WeatherCore_Setup`** is created
   - IF IT DOES NOT CONNECT AUTOMATICALLY TURN OFF MOBILE DATA AND FOLOW INSTRUCTIONS BELOW
   - Connect your phone or PC to that network, then open **`192.168.4.1`** in your browser
   - Enter your WiFi credentials, choose a NOAA satellite view, and tap **Save & Connect**
   - The portal closes, the device connects to your WiFi, and the satellite image appears

4. **On subsequent boots** the device skips the portal and connects automatically using saved settings.  
   To change your WiFi or camera, **hold the BOOT button** during the first 3 seconds after power-on — the setup portal will reopen.

> ⚠️ ESP32 only supports **2.4 GHz** WiFi networks.

> 💾 WiFi credentials and camera choice are saved to flash (NVS) and survive power cycles.

---

## Dependencies

Managed automatically by PlatformIO:

| Library | Author | Purpose |
|---|---|---|
| [GFX Library for Arduino](https://github.com/moononournation/Arduino_GFX) @ 1.4.7 | moononournation | ILI9341 display driver |
| [JPEGDEC](https://github.com/bitbank2/JPEGDEC) | bitbank2 | In-memory JPEG decoding |
| [XPT2046_Touchscreen](https://github.com/PaulStoffregen/XPT2046_Touchscreen) | paulstoffregen | CYD touch (included for compatibility) |
TOUCHSCREEN NOT USED.

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

To change your view, HOLD BOOT BUTTON FOR ABOUT 5 SECONDS WHEN IMAGE IS DISPLAYED 
OR YOU CAN reboot the device and hold the BOOT button within 3 seconds to reopen the portal.

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
