#include "ResetButton.h"

ResetButton::ResetButton(int buttonPin, unsigned long pressDuration) {
  this->buttonPin = buttonPin;
  this->pressDuration = pressDuration;
}

/**
 * Checks if a button connected to the given pin is currently being held 
 * for a duration longer than LONG_PRESS_DURATION (5 seconds).
 * * @param buttonPin The digital pin the button is connected to.
 * @return true if the button has been held down for > 5 seconds, false otherwise.
 */
bool ResetButton::isLongPress() {
  // Read the current state of the button (assuming a pull-up or INPUT_PULLUP configuration, 
  // where LOW means pressed). Adjust logic if using an external pull-down resistor.
  int buttonState = digitalRead(buttonPin);

  // --- Step 1: Detect Start of Press ---
  if (buttonState == LOW) {
    // Button is currently pressed.

    // If it's the very first time we've detected the press (pressStartTime is 0), 
    // record the current time.
    if (pressStartTime == 0) {
      pressStartTime = millis();
    } 
    
    // --- Step 2: Check for Long Press Duration ---
    // Calculate the elapsed time since the press started.
    unsigned long elapsedTime = millis() - pressStartTime;
    // Serial.print("pressed; elapsedTime: ");
    // Serial.println(elapsedTime);

    if (elapsedTime >= pressDuration) {
      // The button has been held for long enough.
      // We return true, but we DO NOT reset pressStartTime here. 
      // Resetting it would cause the function to return false on the next call 
      // even if the button is still held down. The reset happens when the button is released.
      return true;
    }
  } 
  // --- Step 3: Detect Button Release ---
  else {
    // Button is currently NOT pressed (HIGH).
    // Reset the start time so the next press can be correctly timed.
    pressStartTime = 0;
    resettingModeOn = false;
  }

  // If we reach here, either the button is not pressed, or it hasn't been 
  // held for the full duration yet.
  return false;
}


bool ResetButton::canPerformAction() {
  if (resettingModeOn == false) {
    resettingModeOn = !resettingModeOn;
    return true;
  } else {
    return false;
  }
}
