// #include "ManageLockState.h"


// ManageLockState::ManageLockState() {}

// /**
//  * Handles the logic based on the current state and the key pressed.
//  */
// String ManageLockState::processKeyPress(char key) {
//   lastActivityTime = millis(); // Reset activity timer on any key press
  
//   if (currentState == STATE_LOCKED) {
//     // Only '*' can transition out of the locked state
//     if (key == '*') {
//       transitionTo(STATE_UNLOCK_READY);
//     }
//     return "";
//   }
  
//   if (currentState == STATE_UNLOCK_READY) {
//     if (key >= '0' && key <= '9') {
//       // Append digit if within max length
//       if (inputBuffer.length() < MAX_PIN_LENGTH) {
//         inputBuffer += key;
//         rgbLed.keyPressedUnlock();
//       }
//     } else if (key == '#') {
//       // Attempt to unlock
//       if (inputBuffer.length() >= MIN_PIN_LENGTH) {
//         if (inputBuffer.equals(storedPin)) {
//           // Success: Unlock the box
//           transitionTo(STATE_UNLOCKED);
//         } else {
//           // Failure: Incorrect PIN
//           rgbLed.blinkRedTimes(3); // Flash Red 3 times
//           transitionTo(STATE_LOCKED);
//         }
//       } else {
//         // PIN too short, remain ready
//         rgbLed.blinkRedTimes(3); // Flash Red 3 times
//       }
//     } else if (key == '*') {
//       // Check for PIN change initiation: * + [CURRENT PIN] + *
//       if (inputBuffer.length() >= MIN_PIN_LENGTH && inputBuffer.length() <= MAX_PIN_LENGTH) {
//         if (inputBuffer.equals(storedPin)) {
//           // Correct PIN entered, transition to change mode
//           transitionTo(STATE_CHANGE_INIT_READY);
//         } else {
//           // Incorrect PIN for change mode
//           rgbLed.blinkRedTimes(3); // Flash Red 3 times
//           transitionTo(STATE_LOCKED);
//         }
//         inputBuffer = "";
//         return "";
//       }
//       // Clear buffer for next input
//       inputBuffer = "";
//       Serial.println("Keep red for unlock ready");
//       rgbLed.redOn(); // Keep red for unlock ready
//     }
//     return "";
//   }

//   if (currentState == STATE_CHANGE_INIT_READY || currentState == STATE_CHANGE_CONFIRM) {
//     if (key >= '0' && key <= '9') {
//       // Append digit if within max length
//       if (inputBuffer.length() < MAX_PIN_LENGTH) {
//         inputBuffer += key;
//         rgbLed.keyPressedChangePin();
//       }
//     } else if (key == '#') {
//       if (inputBuffer.length() >= MIN_PIN_LENGTH && inputBuffer.length() <= MAX_PIN_LENGTH) {
//         if (currentState == STATE_CHANGE_INIT_READY) {
//           // First entry complete, store it and ask for confirmation
//           changePinFirstAttempt = inputBuffer;
//           // inputBuffer = tempPin; // Store the first attempt temporarily (or use a dedicated variable)
//           transitionTo(STATE_CHANGE_CONFIRM);
//         } else if (currentState == STATE_CHANGE_CONFIRM) {
//           // Second entry complete, compare with stored first entry
//           // String firstPin = String(EEPROM.read(PIN_ADDRESS), inputBuffer.length()); // This is simplified for space, usually requires a separate global temp variable

//           // WARNING: Direct comparison is complex without a global temp variable,
//           // so for this example, we'll use a simplified check assuming the EEPROM
//           // is temporarily storing the first attempt *or* use a fixed temp array.
//           // For a real-world application, use a global `String tempNewPin;`
          
//           if (inputBuffer.equals(changePinFirstAttempt)) { // Simplified mock check
//              // Let's assume a global String tempNewPin was used and holds the first entry
//              // if (inputBuffer.equals(tempNewPin)) {
            
//             // Success: PINs match, save new PIN
//             Serial.print("saving new PIN: ");
//             Serial.println(inputBuffer);

//             String tempPin = String(inputBuffer);
//             managePin.saveNewPin(inputBuffer);
//             rgbLed.blinkGreenTimes(2); // Flash Green twice
//             transitionTo(STATE_LOCKED);
//             inputBuffer = ""; // Clear buffer
//             return tempPin;
//           } else {
//             Serial.println("PIN mismatch");
//             // Mismatch
//             rgbLed.blinkRedTimes(3); // Flash Red quickly
//             transitionTo(STATE_LOCKED);
//             return "";
//           }
//         }
//       } else {
//         Serial.println("PIN too short/long");
//         // PIN too short/long
//         rgbLed.blinkRedTimes(3); // Flash Red quickly
//         transitionTo(STATE_LOCKED);
//         return "";
//       }
//       inputBuffer = ""; // Clear buffer after confirmation attempt
//     } else if (key == '*') {
//       // Cancel/Reset buffer
//       inputBuffer = "";
//       Serial.println("Cancel/Reset buffer - Keep blue");
//       rgbLed.blueOn(); // Keep blue
//     }
//     Serial.print("inputBuffer: ");
//     Serial.println(inputBuffer);
//     return "";
//   }
  
//   if (currentState == STATE_UNLOCKED) {
//     // In unlocked state, any '*' or '#' will re-lock instantly
//     if (key == '*' || key == '#') {
//       transitionTo(STATE_LOCKED);
//     }
//     // Ignore other keys while unlocked
//   }
// }



// /**
//  * Manages state transitions and sets initial state parameters.
//  */
// void ManageLockState::transitionTo(LockState newState) {
//   currentState = newState;
//   inputBuffer = ""; // Clear buffer on state change
//   lastActivityTime = millis(); // Reset timer

//   switch (currentState) {
//     case STATE_LOCKED:
//       lockControl(false);
//       rgbLed.turnOff(); // LED Off
//       Serial.println("State: LOCKED");
//       break;
//     case STATE_UNLOCK_READY:
//       lockControl(false);
//       rgbLed.redOn(); // Solid Red
//       Serial.println("State: UNLOCK_READY (Enter PIN)");
//       break;
//     case STATE_CHANGE_INIT_READY:
//       lockControl(false);
//       Serial.println("blue light...");
//       rgbLed.blueOn(); // Solid Blue
//       Serial.println("State: CHANGE_INIT_READY (Enter NEW PIN)");
//       break;
//     case STATE_CHANGE_CONFIRM:
//       lockControl(false);
//       // LED blinking is handled in loop()
//       Serial.println("State: CHANGE_CONFIRM (Confirm NEW PIN)");
//       break;
//     case STATE_UNLOCKED:
//       lockControl(true);
//       rgbLed.greenOn(); // Solid Green
//       Serial.println("State: UNLOCKED (5s countdown)");
//       // The timeout logic in checkTimeout() will re-lock after 5s
//       break;
//   }
// }
