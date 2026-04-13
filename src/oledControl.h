#ifndef OLED_CONTROL_H
#define OLED_CONTROL_H

#include "OledConfig.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_SH110X.h>

// Displays
Adafruit_SSD1306 displayMux(128, 32, &Wire, -1);
Adafruit_SH1107 displaySH(128, 128, &Wire1, -1);

// Mappings
const char* BTN_LABELS[8] = {
  "SOLO", 
  "FX", 
  "REVERB", 
  "MUTE", 
  "BOOST", 
  "MOD", 
  "DELAY", 
  "DELAY-2"
};

// TCA helpers
bool tcaSelect(uint8_t chan) {
  if (chan > 7) return false;
  Wire.beginTransmission(TCA_ADDR);
  Wire.write(1 << chan);
  byte res = Wire.endTransmission();
  delayMicroseconds(100); 
  return (res == 0);
}

// Init hardware only
bool initOledOnChannel(uint8_t chan) {
  if (!tcaSelect(chan)) return false;
  
  if (!displayMux.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR, false, false)) return false;
  displayMux.clearDisplay();
  displayMux.display(); 
  return true;
}

// Clear specific label
void clearButtonLabel(uint8_t chan) {
  if (!tcaSelect(chan)) return;
  displayMux.clearDisplay();
  displayMux.display();
}

// Show label (Centered)
void showButtonLabel(uint8_t chan) {
  if (!tcaSelect(chan)) return;

  displayMux.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR, false, false);
  displayMux.clearDisplay();
  displayMux.setRotation(2); 
  displayMux.setTextSize(2);
  displayMux.setTextColor(SSD1306_WHITE);

  int16_t x1, y1;
  uint16_t w, h;
  displayMux.getTextBounds(BTN_LABELS[chan], 0, 0, &x1, &y1, &w, &h);

  int x = (128 - w) / 2;
  if (x < 0) x = 0; 

  displayMux.setCursor(x, 8); 
  displayMux.print(BTN_LABELS[chan]);
  displayMux.display();
}

// Centered Word Wrap for SH1107
void printCenteredWordWrap(String text, int y, int size, int maxWidth) {
  int charW = 6 * size; 
  int lineH = 8 * size + 4; 
  int maxCharsPerLine = maxWidth / charW;

  int ptr = 0;
  int len = text.length();

  while (ptr < len) {
    int lineStart = ptr;
    int lineEnd = ptr;
    int currentLen = 0;

    while(lineEnd < len) {
        int nextSpace = text.indexOf(' ', lineEnd);
        if (nextSpace == -1) nextSpace = len;

        int wordLen = nextSpace - lineEnd;
        int addedLen = wordLen + (currentLen > 0 ? 1 : 0);

        if (currentLen + addedLen <= maxCharsPerLine) {
          currentLen += addedLen;
          lineEnd = nextSpace + 1; 
        } else {
          if(currentLen == 0) lineEnd = nextSpace + 1; 
          break; 
        }
    }

    String lineStr = text.substring(lineStart, lineEnd - 1); 
    lineStr.trim();

    int16_t x1, y1;
    uint16_t w, h;
    displaySH.setTextSize(size);
    displaySH.getTextBounds(lineStr, 0, 0, &x1, &y1, &w, &h);

    int x = (maxWidth - w) / 2;

    displaySH.setCursor(x, y);
    displaySH.print(lineStr);

    y += lineH;
    ptr = lineEnd;
  }
}

// Main Display Update
void updateMainDisplay(String presetName, int presetNum) {
  displaySH.clearDisplay();
  displaySH.setTextColor(SH110X_WHITE);
  
  // Header
  displaySH.setTextSize(1);
  displaySH.setCursor(30, 0);
  displaySH.println("Katana MKII");
  displaySH.drawLine(0, 10, 128, 10, SH110X_WHITE);

  // Preset Number
  displaySH.setTextSize(3);
  String pLabel = "P:" + String(presetNum);
  int16_t x1, y1;
  uint16_t w, h;
  displaySH.getTextBounds(pLabel, 0, 0, &x1, &y1, &w, &h);
  int x = (128 - w) / 2;
  
  displaySH.setCursor(x, 20);
  displaySH.print(pLabel);

  // Preset Name
  printCenteredWordWrap(presetName, 60, 2, 128);
  
  displaySH.display();
}

// Init Main Display
bool initSH1107() {
  if (!displaySH.begin(OLED_ADDR, true)) return false;
  displaySH.setRotation(1);
  updateMainDisplay("No USB", 0);
  return true;
}

void oledSetup() {
  delay(400); 
  Wire.begin(SDA_MUX, SCL_MUX);
  Wire.setClock(400000);
  Wire1.begin(SDA2, SCL2);
  Wire1.setClock(400000);

  initSH1107();

  for (uint8_t ch = 0; ch < 8; ch++) {
    initOledOnChannel(ch);
  }
}

#endif
