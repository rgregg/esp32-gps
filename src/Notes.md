/*
spi:
  clk_pin: GPIO12
  miso_pin: GPIO13
  mosi_pin: GPIO11
serial:
  U1 RXD - GPIO18
  U1 TXD - GPIO17


binary_sensor:
  - platform: gpio
    pin: GPIO0
    name: "Button 1"
    filters:
      - invert:
  - platform: gpio
    pin: GPIO14
    name: "Button 2"
    filters:
      - invert:
output:
  - platform: ledc
    pin: GPIO38
    id: gpio38
    frequency: 2000
light:
  - platform: monochromatic
    output: gpio38
    name: "Backlight"
    restore_mode: RESTORE_DEFAULT_ON
*/