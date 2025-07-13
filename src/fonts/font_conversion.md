# Font Conversion

Using the Adafruit GFX fontconvert utility: https://github.com/adafruit/Adafruit-GFX-Library/tree/master/fontconvert.
Which depends on the Freetype library: https://download.savannah.gnu.org/releases/freetype/

Convert the full range of the font from space (32) to the end of the ascii printable range (254) to make sure
we get any special characters we use for the display.

```
fontconvert futura_medium_bt.ttf 12 32 254
```
