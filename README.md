# Nomaduino GPS Monitor

![The Nomadiuno](original_images/nomaduino-92x101.png)

Pronouced _Nomad-duino_ (yes, the second d isn't there).

A small project that leverages an ESP32-S3 and the Adafruit Ultimate GPS board to 
create a location monitoring device that reports GPS NMEA messages over UDP.

## Bill of Materials

- [LilyGO T-Display-S3](https://lilygo.cc/products/t-display-s3)
- [Adafruit Ultimate GPS breakout](https://www.adafruit.com/product/746)

## Scenario

I wanted a way to get higher quality GPS signals into my [Signal K server](https://signalk.org/) than
what I could find from run-of-the-mill UBS GPS devices. In particular, I wanted something that could
get more than 1Hz fix updates from the GPS sensor. The Adafruit Ultimate GPS provides 5Hz fix updates
with 10Hz data updates (although I'm not sure why...) which met what I was looking for.

Since I had a the LilyGo board sitting around from another project, I decided to reuse it for this one
and leverage the screen to provide more information in addition to relaying the GPS signals over a UDP
connection back to the Signal K server. And thus, Nomadiuno was born.

## To Do List

- [ ] Getting started guide
- [ ] Enter WiFi portal mode when there is no WiFi network available for longer than X seconds
- [ ] Acquire time from NTP if we don't have GPS time available yet