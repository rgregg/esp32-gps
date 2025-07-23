# Nomaduino GPS Monitor

![The Nomadiuno](original_images/nomaduino-92x101.png)

Pronouced _Nomad-duino_ (yes, the second d isn't there).

A small project that leverages an ESP32-S3 and the Adafruit Ultimate GPS board to 
create a location monitoring device that reports GPS NMEA messages over UDP.

## Features

- GPS receiver configuration - baud, fix rate, update, rate, data sets
- WiFi connectivity
- Configuration portal
- Telnet logging and commanding
- UDP NMEA 0183 sentence publishing

## Bill of Materials

Original reference hardware:

- [LilyGO T-Display-S3](https://lilygo.cc/products/t-display-s3)
- [Adafruit Ultimate GPS breakout](https://www.adafruit.com/product/746)

ST7789-based color display, 320x170 TFT, 2-buttons

Alternative hardware:

- [Unexpected Maker FeatherS3](https://www.adafruit.com/product/5399)
- [Adafruit FeatherWing OLED Display](https://www.adafruit.com/product/4650)
- [Adafruit Ultimate GPS FeatherWing](https://www.adafruit.com/product/3133)
- [Adafruit FeatherWing Quad](https://www.adafruit.com/product/4253)

SH1107-based monochrome display, 128x64 OLED, 3-buttons

## Scenario

I wanted a way to get higher quality GPS signals into my [Signal K server](https://signalk.org/) than
what I could find from run-of-the-mill UBS GPS devices. In particular, I wanted something that could
get more than 1Hz fix updates from the GPS sensor. The Adafruit Ultimate GPS provides 5Hz fix updates
with 10Hz data updates (although I'm not sure why...) which met what I was looking for.

Since I had a the LilyGo board sitting around from another project, I decided to reuse it for this one
and leverage the screen to provide more information in addition to relaying the GPS signals over a UDP
connection back to the Signal K server. And thus, Nomadiuno was born.
