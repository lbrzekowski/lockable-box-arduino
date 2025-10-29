#include <EEPROM.h>
#include "Keypad.h"
#include "RgbLed.h"
#include "StorePinCode.h"
#include "ResetButton.h"

/* PIN DEFINITION */
#define LED_RED_PIN      9   // PWM pin recommended
#define LED_GREEN_PIN    10  // PWM pin recommended
#define LED_BLUE_PIN     11  // PWM pin recommended
#define LOCK_PIN         12  // Digital pin for the electric lock solenoid (via transistor/relay)
#define RESET_BUTTON_PIN 13

/* EEPROM data */
const byte PIN_MANAGE_EEPROM_ADDRESS = 0;

/* Keypad Dimensions */
const byte ROWS = 4;
const byte COLS = 3;

const byte numPadRowPins[ROWS] = {2, 3, 4, 5}; 
const byte numPadColPins[COLS] = {6, 7, 8}; 

/* NumPad Key Mapping */
char numPadKeys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};

String storedPin = String("");

// // --- PIN & STATE CONFIGURATION ---
const int MIN_PIN_LENGTH = 4;
const int MAX_PIN_LENGTH = 8;
const unsigned long TIMEOUT_UNLOCK_MS = 10000; // 15 seconds for unlock
const unsigned long TIMEOUT_CHANGE_INIT_MS = 20000; // 20 seconds for new PIN entry
const unsigned long TIMEOUT_CHANGE_CONFIRM_MS = 15000; // 15 seconds for confirmation

/* Timer variables */
const long timeoutDuration = 5000;
unsigned long startTime = 0;
bool isTimerActive = false;

// Timing and Input variables
String inputBuffer = "";
String changePinFirstAttempt = "";
unsigned long lastActivityTime = 0;

const unsigned long LONG_PRESS_DURATION = 5000;

Keypad keypad = Keypad(makeKeymap(numPadKeys), numPadRowPins, numPadColPins, ROWS, COLS);
RgbLed rgbLed = RgbLed(LED_RED_PIN, LED_GREEN_PIN, LED_BLUE_PIN);
ManagePinCode managePin = ManagePinCode(PIN_MANAGE_EEPROM_ADDRESS);
ResetButton resetButton = ResetButton(RESET_BUTTON_PIN, LONG_PRESS_DURATION);

bool resettingModeOn = false;

// System States
enum LockState {
  STATE_LOCKED,           // Waiting for '*'
  STATE_UNLOCK_READY,     // Red LED, waiting for PIN entry
  STATE_CHANGE_INIT_READY, // Blue LED, waiting for new PIN (first entry)
  STATE_CHANGE_CONFIRM,   // Blinking Blue, waiting for new PIN confirmation
  STATE_UNLOCKED          // Green LED, lock is open
};
LockState currentState = STATE_LOCKED;

void setup() {
  // Initialize LED pins as outputs
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);

  // Initialize Lock pin
  pinMode(LOCK_PIN, OUTPUT);
  lockControl(false); // Ensure lock is closed on startup

  // Initialize button pin
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);

  Serial.begin(9600);
  
  // Load PIN from EEPROM or use default
  storedPin = managePin.loadPin(); 

  // Set initial LED state (Off/Red to start)
  rgbLed.blinkRedTimes(5); //flashLed(5, 150, 255, 0, 0);
  
  transitionTo(STATE_LOCKED);
}

void loop() {
  // Check for new key press
  char key = keypad.getKey();
  if (key != NO_KEY) {
    String newPin = handleKey(key);
    if (newPin.length() > 0) {
      Serial.print("new pin: ");
      Serial.println(newPin);
      storedPin = newPin;
    }
  }
  
  // Handle system timeouts based on current state
  checkTimeout();
  
  // Custom behavior for LED during STATE_CHANGE_CONFIRM (blinking)
  // if (currentState == STATE_CHANGE_CONFIRM) {
  //   // Fast blinking blue for confirmation wait
  //   if (millis() % 500 < 250) {
  //     rgbLed.blueOn(); // Blue on
  //   } else {
  //     rgbLed.turnOff();   // Off
  //   }
  // }

  if (resetButton.isLongPress()) {
    // NOTE: If you only want the long press action to happen once per press 
    // (not repeatedly while held), you would implement a flag here.
    if (resetButton.canPerformAction()) {
      Serial.println("!!! LONG PRESS DETECTED (Over 5 seconds) !!!");
      storedPin = managePin.resetToDefault();
    }
  }
}

// --- CORE LOGIC FUNCTIONS ---

/**
 * Handles the logic based on the current state and the key pressed.
 */
String handleKey(char key) {
  lastActivityTime = millis(); // Reset activity timer on any key press
  
  if (currentState == STATE_LOCKED) {
    // Only '*' can transition out of the locked state
    if (key == '*') {
      transitionTo(STATE_UNLOCK_READY);
    }
    return "";
  }
  
  if (currentState == STATE_UNLOCK_READY) {
    if (key >= '0' && key <= '9') {
      // Append digit if within max length
      if (inputBuffer.length() < MAX_PIN_LENGTH) {
        inputBuffer += key;
        rgbLed.keyPressedUnlock();
      }
    } else if (key == '#') {
      // Attempt to unlock
      if (inputBuffer.length() >= MIN_PIN_LENGTH) {
        if (inputBuffer.equals(storedPin)) {
          // Success: Unlock the box
          transitionTo(STATE_UNLOCKED);
        } else {
          // Failure: Incorrect PIN
          rgbLed.blinkRedTimes(3); // Flash Red 3 times
          transitionTo(STATE_LOCKED);
        }
      } else {
        // PIN too short, remain ready
        rgbLed.blinkRedTimes(3); // Flash Red 3 times
      }
    } else if (key == '*') {
      // Check for PIN change initiation: * + [CURRENT PIN] + *
      if (inputBuffer.length() >= MIN_PIN_LENGTH && inputBuffer.length() <= MAX_PIN_LENGTH) {
        if (inputBuffer.equals(storedPin)) {
          // Correct PIN entered, transition to change mode
          transitionTo(STATE_CHANGE_INIT_READY);
        } else {
          // Incorrect PIN for change mode
          rgbLed.blinkRedTimes(3); // Flash Red 3 times
          transitionTo(STATE_LOCKED);
        }
        inputBuffer = "";
        return "";
      }
      // Clear buffer for next input
      inputBuffer = "";
      Serial.println("Keep red for unlock ready");
      rgbLed.redOn(); // Keep red for unlock ready
    }
    return "";
  }

  if (currentState == STATE_CHANGE_INIT_READY || currentState == STATE_CHANGE_CONFIRM) {
    if (key >= '0' && key <= '9') {
      // Append digit if within max length
      if (inputBuffer.length() < MAX_PIN_LENGTH) {
        inputBuffer += key;
        rgbLed.keyPressedChangePin();
      }
    } else if (key == '#') {
      if (inputBuffer.length() >= MIN_PIN_LENGTH && inputBuffer.length() <= MAX_PIN_LENGTH) {
        if (currentState == STATE_CHANGE_INIT_READY) {
          // First entry complete, store it and ask for confirmation
          changePinFirstAttempt = inputBuffer;
          // inputBuffer = tempPin; // Store the first attempt temporarily (or use a dedicated variable)
          transitionTo(STATE_CHANGE_CONFIRM);
        } else if (currentState == STATE_CHANGE_CONFIRM) {
          // Second entry complete, compare with stored first entry
          // String firstPin = String(EEPROM.read(PIN_ADDRESS), inputBuffer.length()); // This is simplified for space, usually requires a separate global temp variable

          // WARNING: Direct comparison is complex without a global temp variable,
          // so for this example, we'll use a simplified check assuming the EEPROM
          // is temporarily storing the first attempt *or* use a fixed temp array.
          // For a real-world application, use a global `String tempNewPin;`
          
          if (inputBuffer.equals(changePinFirstAttempt)) { // Simplified mock check
             // Let's assume a global String tempNewPin was used and holds the first entry
             // if (inputBuffer.equals(tempNewPin)) {
            
            // Success: PINs match, save new PIN
            Serial.print("saving new PIN: ");
            Serial.println(inputBuffer);

            String tempPin = String(inputBuffer);
            managePin.saveNewPin(inputBuffer);
            rgbLed.blinkGreenTimes(2); // Flash Green twice
            transitionTo(STATE_LOCKED);
            inputBuffer = ""; // Clear buffer
            return tempPin;
          } else {
            Serial.println("PIN mismatch");
            // Mismatch
            rgbLed.blinkRedTimes(3); // Flash Red quickly
            transitionTo(STATE_LOCKED);
            return "";
          }
        }
      } else {
        Serial.println("PIN too short/long");
        // PIN too short/long
        rgbLed.blinkRedTimes(3); // Flash Red quickly
        transitionTo(STATE_LOCKED);
        return "";
      }
      inputBuffer = ""; // Clear buffer after confirmation attempt
    } else if (key == '*') {
      // Cancel/Reset buffer
      inputBuffer = "";
      Serial.println("Cancel/Reset buffer - Keep blue");
      rgbLed.blueOn(); // Keep blue
    }
    Serial.print("inputBuffer: ");
    Serial.println(inputBuffer);
    return "";
  }
  
  if (currentState == STATE_UNLOCKED) {
    // In unlocked state, any '*' or '#' will re-lock instantly
    if (key == '*' || key == '#') {
      transitionTo(STATE_LOCKED);
    }
    // Ignore other keys while unlocked
  }
}

/**
 * Check if the system has timed out and needs to revert to STATE_LOCKED.
 */
void checkTimeout() {
  unsigned long timeout = 0;
  bool shouldTimeout = false;

  switch (currentState) {
    case STATE_UNLOCK_READY:
      timeout = TIMEOUT_UNLOCK_MS;
      shouldTimeout = true;
      break;
    case STATE_CHANGE_INIT_READY:
      timeout = TIMEOUT_CHANGE_INIT_MS;
      shouldTimeout = true;
      break;
    case STATE_CHANGE_CONFIRM:
      timeout = TIMEOUT_CHANGE_CONFIRM_MS;
      shouldTimeout = true;
      break;
    case STATE_UNLOCKED:
      timeout = 5000; // Lock is open for 5 seconds
      shouldTimeout = true;
      break;
    default:
      shouldTimeout = false;
      break;
  }

  if (shouldTimeout && (millis() - lastActivityTime >= timeout)) {
    transitionTo(STATE_LOCKED);
  }
}

/**
 * Controls the electric lock solenoid.
 */
void lockControl(bool open) {
  if (open) {
    digitalWrite(LOCK_PIN, HIGH); // Assuming HIGH opens the lock
    Serial.println("LOCK: OPEN");
  } else {
    digitalWrite(LOCK_PIN, LOW); // Assuming LOW closes/locks the lock
    Serial.println("LOCK: CLOSED");
  }
}

/**
 * Manages state transitions and sets initial state parameters.
 */
void transitionTo(LockState newState) {
  currentState = newState;
  inputBuffer = ""; // Clear buffer on state change
  lastActivityTime = millis(); // Reset timer

  switch (currentState) {
    case STATE_LOCKED:
      lockControl(false);
      rgbLed.turnOff(); // LED Off
      Serial.println("State: LOCKED");
      break;
    case STATE_UNLOCK_READY:
      lockControl(false);
      rgbLed.redOn(); // Solid Red
      Serial.println("State: UNLOCK_READY (Enter PIN)");
      break;
    case STATE_CHANGE_INIT_READY:
      lockControl(false);
      Serial.println("blue light...");
      rgbLed.blueOn(); // Solid Blue
      Serial.println("State: CHANGE_INIT_READY (Enter NEW PIN)");
      break;
    case STATE_CHANGE_CONFIRM:
      lockControl(false);
      // LED blinking is handled in loop()
      Serial.println("State: CHANGE_CONFIRM (Confirm NEW PIN)");
      break;
    case STATE_UNLOCKED:
      lockControl(true);
      rgbLed.greenOn(); // Solid Green
      Serial.println("State: UNLOCKED (5s countdown)");
      // The timeout logic in checkTimeout() will re-lock after 5s
      break;
  }
}



