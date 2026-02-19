# WeatherCore — NOAA GOES Satellite Image for CYD

A live NOAA GOES-East satellite image viewer running on the **CYD (Cheap Yellow Display)** ESP32 board. Fetches and displays a fresh satellite image every 5 minutes directly from NOAA's public CDN — no API key required.

![CYD displaying NOAA GOES-East CONUS GeoColor satellite image](https://cdn.star.nesdis.noaa.gov/GOES16/ABI/CONUS/GEOCOLOR/416x250.jpg)

---

## What it does

- Connects to your WiFi network on boot
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
│   └── main.cpp          — WiFi, fetch loop, JPEG decode, display
└── include/
    ├── HTTPS.h           — WiFiClientSecure HTTPS GET with chunked transfer support
    └── JPEG.h            — JPEGDEC instance
```

---

## Setup

1. **Clone or download** this project and open it in VS Code with PlatformIO installed.

2. **Edit your WiFi credentials** in `src/main.cpp` (lines 5–6):
   ```cpp
   const char *SSID_NAME = "YourNetworkName";
   const char *SSID_PASSWORD = "YourPassword";
   ```
   > ⚠️ ESP32 only supports **2.4 GHz** WiFi networks.

3. **Build and upload:**
   ```
   pio run --target upload
   ```

4. The display will show status messages as it boots, connects, and fetches the image.

---

## Dependencies

Managed automatically by PlatformIO:

| Library | Author | Purpose |
|---|---|---|
| [GFX Library for Arduino](https://github.com/moononournation/Arduino_GFX) @ 1.4.7 | moononournation | ILI9341 display driver |
| [JPEGDEC](https://github.com/bitbank2/JPEGDEC) | bitbank2 | In-memory JPEG decoding |
| [XPT2046_Touchscreen](https://github.com/PaulStoffregen/XPT2046_Touchscreen) | paulstoffregen | CYD touch (included for compatibility) |

---

## Image Source

**NOAA GOES-16 (GOES-East) — CONUS GeoColor**
`https://cdn.star.nesdis.noaa.gov/GOES16/ABI/CONUS/GEOCOLOR/416x250.jpg`

- Free, public, no authentication required
- Updated approximately every 5 minutes
- Covers the Continental United States and surrounding region
- [NOAA GOES Image Viewer](https://www.star.nesdis.noaa.gov/goes/index.php)

### Changing the satellite view

Edit the `SATELLITE_URL` line in `src/main.cpp` (line ~10) and replace it with any URL from the table below:

```cpp
// Example: switch to GOES-West for a better view of the western US
const char *SATELLITE_URL = "https://cdn.star.nesdis.noaa.gov/GOES18/ABI/CONUS/GEOCOLOR/416x250.jpg";
```

| Satellite | Coverage | URL |
|---|---|---|
| **GOES-East CONUS** *(default)* | Full continental US, eastern focus | `https://cdn.star.nesdis.noaa.gov/GOES16/ABI/CONUS/GEOCOLOR/416x250.jpg` |
| **GOES-West CONUS** | Full continental US, western focus (great for CO, CA, etc.) | `https://cdn.star.nesdis.noaa.gov/GOES18/ABI/CONUS/GEOCOLOR/416x250.jpg` |
| **GOES-East Full Disk** | Americas + Atlantic, full Earth view | `https://cdn.star.nesdis.noaa.gov/GOES16/ABI/FD/GEOCOLOR/678x678.jpg` |
| **GOES-West Full Disk** | Pacific + Americas, full Earth view | `https://cdn.star.nesdis.noaa.gov/GOES18/ABI/FD/GEOCOLOR/678x678.jpg` |
| **GOES-East Caribbean** | Gulf of Mexico + Caribbean | `https://cdn.star.nesdis.noaa.gov/GOES16/ABI/SECTOR/car/GEOCOLOR/416x250.jpg` |
| **GOES-East Alaska** | Alaska region | `https://cdn.star.nesdis.noaa.gov/GOES18/ABI/SECTOR/ak/GEOCOLOR/416x250.jpg` |
| **GOES-West Hawaii** | Hawaii and surrounding Pacific | `https://cdn.star.nesdis.noaa.gov/GOES18/ABI/SECTOR/hi/GEOCOLOR/416x250.jpg` |

> **Note:** The Full Disk URLs use a 678×678 image. For best results with those, change the decode line in `loop()` to:
> ```cpp
> jpeg.decode(-179 /* x */, -179 /* y */, JPEG_SCALE_HALF /* options */);
> ```
> which scales it to 339×339 and crops to the center of the 320×240 screen.

---

## Credits & Acknowledgements

This project is a port and adaptation of the following open-source works:

### [WeatherPanel](https://github.com/moononournation/WeatherPanel) by moononournation
The original **WeatherSatelliteImage** Arduino sketch that inspired this project. It targets a different ESP32 board (AXS15231B QSPI 172×640 display) and fetches FY-4B satellite imagery from China's NMC weather service. The core HTTPS fetch logic (`HTTPS.h`) and JPEG decode callback pattern come directly from this project.

> Original designed for the FengYun FY-4B geostationary satellite over Asia. Adapted here for NOAA GOES-East and the CYD hardware platform.

### [AntiPMatrix](https://github.com) — CYD Framework Reference
The CYD board pin configuration, PlatformIO setup, and proven working display/SPI configuration used in this project were derived from the **AntiPMatrix** CYD project. The ILI9341 hardware SPI pinout, backlight GPIO, and library versions were taken directly from this reference.

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
