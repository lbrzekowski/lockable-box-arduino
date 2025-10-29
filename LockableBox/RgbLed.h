#ifndef RGBLED_H
#define RGBLED_H

#include <Arduino.h>

class RgbLed {
public:
  RgbLed(byte redPin, byte greenPin, byte bluePin);
  void redOn();
  void greenOn();
  void blueOn();
  void turnOff();
  void keyPressedUnlock();
  void keyPressedChangePin();
  void blinkRedTimes(byte times);
  void blinkGreenTimes(byte times);
private:
  byte redPin;
  byte greenPin;
  byte bluePin;
  void setLedColor(int r, int g, int b);
  void flashLed(int count, int delayMs, int r, int g, int b);
};

#endif