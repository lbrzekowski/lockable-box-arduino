#include "StorePinCode.h"

/**
 * PIN code is maximum 8 digit long. It takes 10 bytes of EEPROM memory.
 * Memery structure:
 * - 1st byte - initialization flag
 * - 2nd byte - lenght of the PIN code
 * - 3 - 10 - space for the PIN code
 */
ManagePinCode::ManagePinCode(byte eepromAddress) {
  this->eepromFlagAddress = eepromAddress;
  this->eepromPinLengthAddress = eepromAddress + 1;
  this->eepromPinCodeAddress = eepromAddress + 2;
}

/**
 * Checks the flag, initializes if needed, and returns the loaded PIN code.
 */
String ManagePinCode::loadPin() {
  byte flag = EEPROM.read(eepromFlagAddress);
  if (flag != PIN_CODE_FLAG) {
    Serial.println("EEPROM not configured. Writing default string.");

    saveNewPin(DEFAULT_PIN_VALUE);
    EEPROM.write(eepromFlagAddress, PIN_CODE_FLAG);
    
    Serial.println("EEPROM initialized with default digits.");
  } 

  String data = "";
  int len = EEPROM.read(eepromPinLengthAddress); 

  for (int i = 0; i < len; i++) {
    char character = EEPROM.read(eepromPinCodeAddress + i);
    data += character;
  }

  Serial.print("PIN loaded from EEPROM: ");
  Serial.println(data);
  return data;
}

void ManagePinCode::saveNewPin(const String& data) {
  int pinLength = data.length();
  EEPROM.put(eepromPinLengthAddress, pinLength);

  for (int i = 0; i < pinLength; i++) {
    EEPROM.write(eepromPinCodeAddress + i, data[i]);
  }
}

String ManagePinCode::resetToDefault() {
  saveNewPin(DEFAULT_PIN_VALUE);
  return String(DEFAULT_PIN_VALUE);
}
