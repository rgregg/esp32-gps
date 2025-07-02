#pragma once

#include <Arduino.h>

// Define default long press duration (in milliseconds)
#define DEFAULT_LONG_PRESS_DURATION 1000

// Define default debounce delay (in milliseconds)
#define DEFAULT_DEBOUNCE_DELAY 50

// Enum for button press types
enum ButtonPressType {
    SHORT_PRESS,
    LONG_PRESS
};

// Callback function type for button presses
typedef void (*ButtonPressCallback)(ButtonPressType);

class ButtonManager {
public:
    ButtonManager(int pin, ButtonPressCallback callback, unsigned long longPressDuration = DEFAULT_LONG_PRESS_DURATION, unsigned long debounceDelay = DEFAULT_DEBOUNCE_DELAY);
    void loop();

private:
    int _pin;
    ButtonPressCallback _callback;
    unsigned long _longPressDuration;
    unsigned long _debounceDelay;

    int _lastButtonState;  // Last reading from the button pin
    unsigned long _lastDebounceTime; // Last time the button pin was toggled
    bool _buttonPressed; // True if the button is currently considered pressed (debounced)
    unsigned long _pressStartTime; // Time when the button press started
    bool _insideLongPress; // True if the button is currently considered a long press
};
