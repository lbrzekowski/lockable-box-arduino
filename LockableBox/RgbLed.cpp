#include "RgbLed.h"

// constructor
RgbLed::RgbLed(byte redPin, byte greenPin, byte bluePin) {
  this->redPin = redPin;
  this->greenPin = greenPin;
  this->bluePin = bluePin;
}

void RgbLed::redOn() {
  setLedColor(255, 0, 0);
}

void RgbLed::greenOn() {
  setLedColor(0, 255, 0);
}

void RgbLed::blueOn() {
  setLedColor(0, 0, 255);
}

void RgbLed::turnOff() {
  setLedColor(0, 0, 0);
}

void RgbLed::keyPressedUnlock() {
  // Visual feedback: brief green flash on keypress
  setLedColor(0, 150, 0); 
  delay(90);
  setLedColor(255, 0, 0); // Back to red
}

void RgbLed::keyPressedChangePin() {
  // Visual feedback: brief blue flash on keypress
  setLedColor(0, 0, 50); 
  delay(50);
  setLedColor(0, 0, 255); // Back to solid blue (or blinking handled by loop)
}

void RgbLed::blinkRedTimes(byte times) {
  flashLed(times, 150, 255, 0, 0);
}

void RgbLed::blinkGreenTimes(byte times) {
  flashLed(times, 150, 0, 255, 0);
}

/**
 * Controls the RGB LED color (Common Cathode: 0 is Max Brightness).
 */
void RgbLed::setLedColor(int r, int g, int b) {
  // Invert values for Common Cathode
  analogWrite(redPin, r);
  analogWrite(greenPin, g);
  analogWrite(bluePin, b);
}

/**
 * Flashes the LED a specified number of times with a color.
 */
void RgbLed::flashLed(int count, int delayMs, int r, int g, int b) {
  int i = 0;
  for (i = 0; i < count; i++) {
    setLedColor(r, g, b);
    delay(delayMs);
    setLedColor(0, 0, 0);
    delay(delayMs);
  }
}
