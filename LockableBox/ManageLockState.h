// #ifndef MANAGE_LOCK_STATE_H
// #define MANAGE_LOCK_STATE_H

// #include <Arduino.h>

// const int MIN_PIN_LENGTH = 4;
// const int MAX_PIN_LENGTH = 8;

// enum LockState {
//   STATE_LOCKED,           // Waiting for '*'
//   STATE_UNLOCK_READY,     // Red LED, waiting for PIN entry
//   STATE_CHANGE_INIT_READY, // Blue LED, waiting for new PIN (first entry)
//   STATE_CHANGE_CONFIRM,   // Blinking Blue, waiting for new PIN confirmation
//   STATE_UNLOCKED          // Green LED, lock is open
// };

// class ManageLockState {
// public:
//   ManageLockState();
//   String processKeyPress(char key);
// private:
//   void transitionTo(LockState newState);
//   unsigned long lastActivityTime = 0;
//   LockState currentState = STATE_LOCKED;
//   String inputBuffer = "";
// };

// #endif