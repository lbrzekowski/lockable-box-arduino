#ifndef STORE_PIN_CODE_H
#define STORE_PIN_CODE_H

#include <Arduino.h>
#include <EEPROM.h>

const byte PIN_CODE_FLAG = 0xAA;
const char DEFAULT_PIN_VALUE[5] = "1111";

class ManagePinCode {
public:
  ManagePinCode(byte eepromAddress);
  String loadPin();
  void saveNewPin(const String& data);
  String resetToDefault();
private:
  byte eepromFlagAddress;
  byte eepromPinLengthAddress;
  byte eepromPinCodeAddress;
};

#endif