#ifndef BUTTON_CONFIG_H
#define BUTTON_CONFIG_H

// =================== LEDs PIN ===================
// Reordered to match the button swap logic
// Group 1 (0-3 Reversed)
static const int LED_0_PIN = 48; // Was LED_3
static const int LED_1_PIN = 47; // Was LED_2
static const int LED_2_PIN = 21; // Was LED_1
static const int LED_3_PIN = 42; // Was LED_0

// Group 2 (4-7 Reversed)
static const int LED_4_PIN = 45; // Was LED_7
static const int LED_5_PIN = 35; // Was LED_6
static const int LED_6_PIN = 36; // Was LED_5
static const int LED_7_PIN = 37; // Was LED_4

// ================== BUTTON PIN ==================
// Reordered based on your log output
// Group 1 (0-3 Reversed)
static const int BTTN_0_PIN = 14; // Was BTTN_3 (Now Index 0)
static const int BTTN_1_PIN = 46; // Was BTTN_2 (Now Index 1)
static const int BTTN_2_PIN = 41; // Was BTTN_1 (Now Index 2)
static const int BTTN_3_PIN = 39; // Was BTTN_0 (Now Index 3)

// Group 2 (4-7 Reversed)
static const int BTTN_4_PIN = 6;  // Was BTTN_7 (Now Index 4)
static const int BTTN_5_PIN = 10; // Was BTTN_6 (Now Index 5)
static const int BTTN_6_PIN = 4;  // Was BTTN_5 (Now Index 6)
static const int BTTN_7_PIN = 40; // Was BTTN_4 (Now Index 7)

// Presets (Unchanged as you didn't report issues here)
static const int BTTN_8_PIN = 38;
static const int BTTN_9_PIN = 7;

#endif