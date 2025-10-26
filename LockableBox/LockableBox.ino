#include <EEPROM.h>
// #include "NumPadReader.h"
#include "Keypad.h"

/* PIN DEFINITION */
#define LED_RED_PIN     9   // PWM pin recommended
#define LED_GREEN_PIN   10  // PWM pin recommended
#define LED_BLUE_PIN    11  // PWM pin recommended
#define LOCK_PIN        12  // Digital pin for the electric lock solenoid (via transistor/relay)

/* EEPROM data */
const byte CONFIG_FLAG_ADDRESS = 0;
const byte CONFIG_FLAG_VALUE = 0xAA;
const byte PASSWORD_LENGTH_ADDRESS = 1;
const byte PASSWORD_ADDRESS = 2;

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

// // EEPROM address for storing the PIN (using the first 8 bytes)

/* Timer variables */
const long timeoutDuration = 5000;
unsigned long startTime = 0;
bool isTimerActive = false;

// Timing and Input variables
String inputBuffer = "";
String changePinFirstAttempt = "";
unsigned long lastActivityTime = 0;

Keypad keypad = Keypad(makeKeymap(numPadKeys), numPadRowPins, numPadColPins, ROWS, COLS);

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

  Serial.begin(9600);
  Serial.println("Keypad Ready (No Library). Press a key...");
  
  // Load PIN from EEPROM or use default
  storedPin = loadPin(); 

  // Set initial LED state (Off/Red to start)
  flashLed(5, 150, 255, 0, 0);
  
  transitionTo(STATE_LOCKED);
}

void loop() {
  // Check for new key press
  char key = keypad.getKey();
  if (key != NO_KEY) {
    handleKey(key);
  }
  
  // Handle system timeouts based on current state
  checkTimeout();
  
  // Custom behavior for LED during STATE_CHANGE_CONFIRM (blinking)
  if (currentState == STATE_CHANGE_CONFIRM) {
    // Fast blinking blue for confirmation wait
    if (millis() % 500 < 250) {
      setLedColor(0, 0, 255); // Blue on
    } else {
      setLedColor(0, 0, 0);   // Off
    }
  }
}

// --- CORE LOGIC FUNCTIONS ---

/**
 * Handles the logic based on the current state and the key pressed.
 */
void handleKey(char key) {
  lastActivityTime = millis(); // Reset activity timer on any key press
  
  if (currentState == STATE_LOCKED) {
    // Only '*' can transition out of the locked state
    if (key == '*') {
      transitionTo(STATE_UNLOCK_READY);
    }
    return;
  }
  
  if (currentState == STATE_UNLOCK_READY) {
    if (key >= '0' && key <= '9') {
      // Append digit if within max length
      if (inputBuffer.length() < MAX_PIN_LENGTH) {
        inputBuffer += key;
        // Visual feedback: brief green flash on keypress
        setLedColor(0, 150, 0); 
        delay(90);
        setLedColor(255, 0, 0); // Back to red
      }
    } else if (key == '#') {
      // Attempt to unlock
      if (inputBuffer.length() >= MIN_PIN_LENGTH) {
        if (inputBuffer.equals(storedPin)) {
          // Success: Unlock the box
          transitionTo(STATE_UNLOCKED);
        } else {
          // Failure: Incorrect PIN
          flashLed(3, 100, 255, 0, 0); // Flash Red 3 times
          transitionTo(STATE_LOCKED);
        }
      } else {
        // PIN too short, remain ready
        flashLed(3, 100, 255, 0, 0); // Flash Red 3 times
      }
    } else if (key == '*') {
      // Check for PIN change initiation: * + [CURRENT PIN] + *
      if (inputBuffer.length() >= MIN_PIN_LENGTH && inputBuffer.length() <= MAX_PIN_LENGTH) {
        if (inputBuffer.equals(storedPin)) {
          // Correct PIN entered, transition to change mode
          transitionTo(STATE_CHANGE_INIT_READY);
        } else {
          // Incorrect PIN for change mode
          flashLed(3, 100, 255, 0, 0); // Flash Red 3 times
          transitionTo(STATE_LOCKED);
        }
        inputBuffer = "";
        return;
      }
      // Clear buffer for next input
      inputBuffer = "";
      Serial.println("Keep red for unlock ready");
      setLedColor(255, 0, 0); // Keep red for unlock ready
    }
    return;
  }

  if (currentState == STATE_CHANGE_INIT_READY || currentState == STATE_CHANGE_CONFIRM) {
    if (key >= '0' && key <= '9') {
      // Append digit if within max length
      if (inputBuffer.length() < MAX_PIN_LENGTH) {
        inputBuffer += key;
        // Visual feedback: brief blue flash on keypress
        setLedColor(0, 0, 50); 
        delay(50);
        setLedColor(0, 0, 255); // Back to solid blue (or blinking handled by loop)
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
            // saveNewPin(inputBuffer);
            storedPin = String(inputBuffer);
            saveNewPin(inputBuffer);
            flashLed(2, 200, 0, 255, 0); // Flash Green twice
            transitionTo(STATE_LOCKED);
          } else {
            Serial.println("PIN mismatch");
            // Mismatch
            flashLed(3, 100, 255, 0, 0); // Flash Red quickly
            transitionTo(STATE_LOCKED);
            return;
          }
        }
      } else {
        Serial.println("PIN too short/long");
        // PIN too short/long
        // flashLed(1, 100, 255, 0, 0); // Brief error flash
        flashLed(3, 100, 255, 0, 0); // Flash Red quickly
        transitionTo(STATE_LOCKED);
        return;
      }
      inputBuffer = ""; // Clear buffer after confirmation attempt
    } else if (key == '*') {
      // Cancel/Reset buffer
      inputBuffer = "";
      Serial.println("Cancel/Reset buffer - Keep blue");
      setLedColor(0, 0, 255); // Keep blue
    }
    Serial.print("inputBuffer: ");
    Serial.println(inputBuffer);
    return;
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

void saveNewPin(const String& data) {
  int len = data.length();
  EEPROM.put(PASSWORD_LENGTH_ADDRESS, len);
  EEPROM.put(PASSWORD_ADDRESS, len);

  for (int i = 0; i < len; i++) {
    EEPROM.write(PASSWORD_ADDRESS + i, data[i]);
  }
}

void initializeEEPROM() {
  Serial.println("EEPROM not configured. Writing default string.");
  
  // 1. Define and write your default string
  String defaultDigits = "1111";
  saveNewPin(defaultDigits);

  // 2. Write the magic flag LAST
  EEPROM.write(CONFIG_FLAG_ADDRESS, CONFIG_FLAG_VALUE);
  
  // EEPROM.commit() if using ESP8266/ESP32
  Serial.println("EEPROM initialized with default digits.");
}

/**
 * Checks the flag, initializes if needed, and returns the loaded String.
 */
String loadPin() {
  byte flag = EEPROM.read(CONFIG_FLAG_ADDRESS);
  if (flag != CONFIG_FLAG_VALUE) {
    initializeEEPROM();
  } 
  
  // Now that we know the EEPROM is configured, read the string data
  Serial.println("Loading configuration string...");

  // Use the read function from our previous exchange
  String data = "";

  // Read length
  int len = EEPROM.read(PASSWORD_LENGTH_ADDRESS); 

  // Read characters
  for (int i = 0; i < len; i++) {
    char character = EEPROM.read(PASSWORD_ADDRESS + i);
    data += character;
  }

  Serial.print("PIN loaded from EEPROM: ");
  Serial.println(data);
  return data;
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
      setLedColor(0, 0, 0); // LED Off
      Serial.println("State: LOCKED");
      break;
    case STATE_UNLOCK_READY:
      lockControl(false);
      setLedColor(255, 0, 0); // Solid Red
      Serial.println("State: UNLOCK_READY (Enter PIN)");
      break;
    case STATE_CHANGE_INIT_READY:
      lockControl(false);
      Serial.println("blue light...");
      setLedColor(0, 0, 255); // Solid Blue
      Serial.println("State: CHANGE_INIT_READY (Enter NEW PIN)");
      break;
    case STATE_CHANGE_CONFIRM:
      lockControl(false);
      // LED blinking is handled in loop()
      Serial.println("State: CHANGE_CONFIRM (Confirm NEW PIN)");
      break;
    case STATE_UNLOCKED:
      lockControl(true);
      setLedColor(0, 255, 0); // Solid Green
      Serial.println("State: UNLOCKED (5s countdown)");
      // The timeout logic in checkTimeout() will re-lock after 5s
      break;
  }
}

// --- UTILITY FUNCTIONS ---

/**
 * Controls the RGB LED color (Common Cathode: 0 is Max Brightness).
 */
void setLedColor(int r, int g, int b) {
  // Invert values for Common Cathode
  analogWrite(LED_RED_PIN, r);
  analogWrite(LED_GREEN_PIN, g);
  analogWrite(LED_BLUE_PIN, b);
}

/**
 * Flashes the LED a specified number of times with a color.
 */
void flashLed(int count, int delayMs, int r, int g, int b) {
  int i = 0;
  for (i = 0; i < count; i++) {
    setLedColor(r, g, b);
    delay(delayMs);
    setLedColor(0, 0, 0);
    delay(delayMs);
  }
}


