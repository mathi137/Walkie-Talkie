#ifndef ROTATORY_ENCODER_HPP
#define ROTATORY_ENCODER_HPP

#include <Arduino.h>

class RotatoryEncoder {
public:
    enum SwitchState {
        PRESSED,
        HELD,
        RELEASED
    };

    RotatoryEncoder(uint8_t switchPin, unsigned long debounceDelay = 50, unsigned long holdTime = 1000) 
        : _switchPin(switchPin), 
          _debounceDelay(debounceDelay), 
          _holdTime(holdTime),
          _previousState(SwitchState::RELEASED),
          _currentState(SwitchState::RELEASED),
          _lastReading(HIGH),
          _lastDebounceTime(0),
          _pressStartTime(0)
    {}

    void begin() {
        pinMode(_switchPin, INPUT_PULLUP);
    }

    void update() {
        int reading = digitalRead(_switchPin);

        // Debounce check
        if (reading != _lastReading) {
            _lastDebounceTime = millis();
        }
        _lastReading = reading;

        if ((millis() - _lastDebounceTime) > _debounceDelay) {
            // Determine pressed or realeased state (LOW means pressed)
            if (reading == LOW && _currentState == SwitchState::RELEASED) {
                _currentState = SwitchState::PRESSED;
                _pressStartTime = millis();
            } else if (reading == LOW && _currentState == SwitchState::PRESSED) {
                if ((millis() - _pressStartTime) > _holdTime) {
                    _currentState = SwitchState::HELD;
                }
            } else if (reading == HIGH && _currentState != SwitchState::RELEASED) {
                _currentState = SwitchState::RELEASED;
            }
        }
    }

    bool isPressed() const { return _currentState == SwitchState::PRESSED; }
    bool isHeld() const { return _currentState == SwitchState::HELD; }
    bool isReleased() const { return _currentState == SwitchState::RELEASED; }

    // For detecting *edges* (transitions)
    bool wasPressed() {
        if (_currentState == SwitchState::PRESSED && _previousState != SwitchState::PRESSED) {
            _previousState = SwitchState::PRESSED;
            return true;
        }
        _previousState = _currentState;
        return false;
    }

    bool wasReleased() {
        if (_currentState == SwitchState::RELEASED && _previousState != SwitchState::RELEASED) {
            _previousState = SwitchState::RELEASED;
            return true;
        }
        _previousState = _currentState;
        return false;
    }

private:
    uint8_t _switchPin;
    unsigned long _debounceDelay;
    unsigned long _holdTime;
    SwitchState _previousState;
    SwitchState _currentState;
    int _lastReading;
    unsigned long _lastDebounceTime;
    unsigned long _pressStartTime;
};

#endif
