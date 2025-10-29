#ifndef RESET_BUTTON_H
#define RESET_BUTTON_H

#include <Arduino.h>

class ResetButton {
public:
  ResetButton(int buttonPin, unsigned long pressDuration);
  bool isLongPress();
  bool canPerformAction();
private:
  int buttonPin;
  unsigned long pressDuration;
  unsigned long pressStartTime = 0;
  bool resettingModeOn = false;
};

#endif