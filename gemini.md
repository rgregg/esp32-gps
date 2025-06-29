# Gemini Project Information

This file provides project-specific information to the Gemini AI assistant.

## Project Overview

This project is a ESP32-S3 based GPS tracker using a LilyGO T-Display S3 board and an Adafruit Ultimate GPS board.
It is built using the Platform.IO environment, with the Arduino framework and several libraries for GPS, JSON, OTA updates, and graphics.

All of the source code files are stored in the /src folder.
All of the data loaded into the filesystem is in the /data folder.
Local web server pages are stored in /data/web.
Unit tests are all stored in the /test folder.

## Build/Run Commands

To build the project, use the following command:

```bash
platformio run -e lilygo-t-display-s3
```

To upload the project to the device, use:

```bash
platformio run -e lilygo-t-display-s3 -t upload
```

Never automatically upload the project without asking the user.

## Testing

Unit tests can be run on the device with the following command:

```bash
platformio test -e lilygo-t-display-s3 
```
