#include <usbhub.h>
#include <usbh_midi.h>
#include "oledControl.h"
#include "buttonController.h"
#include "KatanaLogic.h"

USB       Usb;
USBH_MIDI Midi(&Usb);

KatanaController katana(&Midi);

bool connected = false;
int currentPreset = 1; 

// --- TIMING CONFIGURATION ---
unsigned long syncTimer = 0;
bool syncPending = false;

// Queue for Preset Changes
unsigned long presetSendTimer = 0; 
int pendingPreset = -1;            

// Wait this long after the LAST click before sending the command
const int PRESET_DELAY_MS = 350; 

void onSysExStateChange(uint8_t index, bool state) {
  if (katana.isScanning) return;
  setLed(index, state);
}

void onSysExNameChange(String name) { }

// Blocking Wait (Startup only)
void waitForPatchLoad(int ms) {
  unsigned long t = millis();
  while(millis() - t < (unsigned long)ms) {
    Usb.Task();
    katana.parseIncoming();
  }
}

// Blocking Sync (Startup only)
void syncEffectsBlocking() {
  for(int i=0; i<8; i++) {
    katana.requestEffectByIndex(i);
    unsigned long t = millis();
    while(millis() - t < 50) { 
      Usb.Task();
      katana.parseIncoming();
    }
  }
}

void scanAllPresetsOnStartup() {
  katana.isScanning = true; 
  katana.resetEffectsState(); 
  katana.effectsState[3] = true;
  updateMainDisplay("Scanning...", 0);

  // Read all names and states from memory
  for(int p=1; p<=8; p++) {
    char buf[16];
    sprintf(buf, "Reading P:%d", p);
    updateMainDisplay(buf, p);

    // Read Name
    katana.requestNameByIndex(p);
    waitForPatchLoad(50); 
    
    // Read Effects
    for(int i=0; i<8; i++) {
       if (i != 0 && i != 3) {
          katana.requestEffectForPreset(p, i);
          waitForPatchLoad(20);
       }
    }
  }

  katana.isScanning = false;

  katana.changePreset(1);
  waitForPatchLoad(400);

  katana.requestVolume();
  waitForPatchLoad(100); 

  katana.loadFromCache(1); 
  katana.effectsState[3] = true;
  if (katana.onEffectChange) katana.onEffectChange(3, true);

  uint8_t muteVal = 0;
  katana.sendSysExChange(EN_TUNER, &muteVal, 1);
  waitForPatchLoad(100);

  currentPreset = 1;
}

void setup() {
  Serial.begin(115200);
  oledSetup(); 
  setupButtons();
  katana.setCallbacks(onSysExStateChange, onSysExNameChange);
  katana.effectsState[3] = true;

  if (Usb.Init() == -1) {
    while (1) delay(1000); // Halt if USB host shield fails to init
  }

  setLed(3, true); // Tuner/Mute led ON initially
}

void loop() {
  Usb.Task();

  bool now = (Midi.GetAddress() != 0);
  
  if (now && !connected) { 
    connected = true; 
    Serial.println("Katana Connected!");

    delay(500);
    katana.setEditorMode(true); 
    delay(100);
    katana.setEditorMode(true);

    delay(1000);

    katana.effectsState[3] = true;
    uint8_t muteVal = 0;
    katana.sendSysExChange(EN_TUNER, &muteVal, 1);
    waitForPatchLoad(100);

    scanAllPresetsOnStartup();

    for(int i=0; i<8; i++) {
      showButtonLabel(i);
    }

    updateMainDisplay(katana.getCachedName(currentPreset), currentPreset);
  }
  
  if (!now && connected) { 
    connected = false; 
    katana.resetEffectsState(); 
    katana.effectsState[3] = true;
    setLed(3, true);
    for(int i=0; i<8; i++) clearButtonLabel(i);
    updateMainDisplay("No USB", 0);
  }

  if (!connected) return;

  // --- BUTTON HANDLING ---
  int btn = scanButtons();
  if (btn != -1) {
    if (btn < 8) {
      katana.toggleEffect(btn); 
    }
    else if (btn == 8 || btn == 9) { 
      // PRESET CHANGE
      if (btn == 8) currentPreset++;
      else currentPreset--;
      
      if (currentPreset < 1) currentPreset = 8;
      if (currentPreset > 8) currentPreset = 1;
      
      // INSTANT UI UPDATE
      katana.loadFromCache(currentPreset);
      updateMainDisplay(katana.getCachedName(currentPreset), currentPreset);
      
      // QUEUE THE COMMAND
      pendingPreset = currentPreset;
      presetSendTimer = millis() + PRESET_DELAY_MS;
      
      syncPending = false; 
    }
  }

  // --- DELAYED COMMAND SENDER ---
  if (pendingPreset != -1 && millis() > presetSendTimer) {
    // Send the actual change command to the Amp
    katana.changePreset(pendingPreset);

    // Give the amp plenty of time to load before asking for status
    syncTimer = millis() + 650;
    syncPending = true;

    pendingPreset = -1;
  }

  // --- BACKGROUND SYNC CHECK ---
  if (syncPending && millis() > syncTimer) {
    syncPending = false;

    katana.requestVolume();
    for(int i=0; i<8; i++) {
      katana.requestEffectByIndex(i);
    }
  }

  katana.parseIncoming();
}
