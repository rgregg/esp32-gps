#include "ButtonManager.h"

ButtonManager::ButtonManager(int pin, ButtonPressCallback callback, unsigned long longPressDuration, unsigned long debounceDelay)
    : _pin(pin),
      _callback(callback),
      _longPressDuration(longPressDuration),
      _debounceDelay(debounceDelay),
      _lastButtonState(HIGH), // Assume button is not pressed initially
      _lastDebounceTime(0),
      _buttonPressed(false),
      _pressStartTime(0)
{
    pinMode(_pin, INPUT_PULLUP); // Buttons are typically wired with pull-up resistors
}

void ButtonManager::loop() {
    int reading = digitalRead(_pin);

    // If the button state has changed, reset the debounce timer
    if (reading != _lastButtonState) {
        _lastDebounceTime = millis();
    }

    // If the debounce delay has passed, and the button state is stable
    if ((millis() - _lastDebounceTime) > _debounceDelay) {
        // If the button is pressed (LOW, assuming pull-up)
        if (reading == LOW) {
            if (!_buttonPressed) {
                // Button just pressed
                _buttonPressed = true;
                _pressStartTime = millis();
            } else {
                // Button is being held down, check for long press
                if ((millis() - _pressStartTime) >= _longPressDuration) {
                    if (_callback && !_insideLongPress) {
                        _insideLongPress = true;
                        _callback(LONG_PRESS);
                    }
                }
            }
        } else {
            // Button is released
            if (_buttonPressed) {
                // Button was pressed, now released. Check for short press
                if ((millis() - _pressStartTime) < _longPressDuration && !_insideLongPress) {
                    if (_callback) {
                        _callback(SHORT_PRESS);
                    }
                }
                _insideLongPress = false;
                _buttonPressed = false;
            }
        }
    }

    _lastButtonState = reading;
}
