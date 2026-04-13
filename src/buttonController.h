#ifndef BUTTON_CONTROLLER_H
#define BUTTON_CONTROLLER_H

#include <Arduino.h>
#include <ButtonConfig.h>

const uint8_t LED_PINS[8] = { LED_0_PIN, LED_1_PIN, LED_2_PIN, LED_3_PIN, LED_4_PIN, LED_5_PIN, LED_6_PIN, LED_7_PIN};
const uint8_t BTN_PINS[10] = { BTTN_0_PIN, BTTN_1_PIN, BTTN_2_PIN, BTTN_3_PIN, BTTN_4_PIN, BTTN_5_PIN, BTTN_6_PIN, BTTN_7_PIN, BTTN_8_PIN, BTTN_9_PIN};

// Debounce params
constexpr uint16_t DEBOUNCE_MS = 30;
bool lastStableBtn[10];
bool lastReadBtn[10];
uint32_t lastChangeMs[10];

void setupButtons() {
  for (uint8_t i = 0; i < 8; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }
  for (uint8_t i = 0; i < 10; i++) {
    pinMode(BTN_PINS[i], INPUT_PULLUP);
    bool initial = digitalRead(BTN_PINS[i]) == LOW;
    lastStableBtn[i] = initial;
    lastReadBtn[i] = initial;
    lastChangeMs[i] = millis();
  }
}

// Helper to set LED physically
void setLed(uint8_t index, bool on) {
  if (index < 8) {
    digitalWrite(LED_PINS[index], on ? HIGH : LOW);
  }
}

// Returns -1 if no press, or 0-9 if pressed
int scanButtons() {
  uint32_t now = millis();
  for (uint8_t i = 0; i < 10; i++) {
    bool reading = (digitalRead(BTN_PINS[i]) == LOW); 
    
    if (reading != lastReadBtn[i]) {
      lastReadBtn[i] = reading;
      lastChangeMs[i] = now;
    }

    if ((now - lastChangeMs[i]) >= DEBOUNCE_MS) {
      if (reading != lastStableBtn[i]) {
        lastStableBtn[i] = reading;
        if (reading == true) {
          return i; // Button Pressed event
        }
      }
    }
  }
  return -1;
}

#endif