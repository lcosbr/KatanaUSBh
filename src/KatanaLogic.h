#ifndef KATANA_LOGIC_H
#define KATANA_LOGIC_H

#include <Arduino.h>
#include <usbh_midi.h>
#include "KatanaDefines.h"

typedef void (*StateCallback)(uint8_t index, bool state);
typedef void (*NameCallback)(String name);

class KatanaController {
public:
  USBH_MIDI* midi;
  StateCallback onEffectChange;
  NameCallback onNameChange;

  uint8_t sysExBuf[128];
  int sysExIdx = 0;

  char cachedNames[9][17]; 
  bool presetCache[9][8];
  bool isScanning = false; 

  // Order: SOLO, FX, REVERB, MUTE, BOOST, MOD, DELAY, DELAY2
  uint32_t effectsAddr[8] = {
    EN_SOLO, 
    EN_FX, 
    EN_REVERB, 
    EN_TUNER, 
    EN_BOOSTER, 
    EN_MOD, 
    EN_DELAY, 
    EN_DELAY2 
  };
  
  bool effectsState[8] = {0}; 
  int activePreset = 1; 

  KatanaController(USBH_MIDI* m) : midi(m) {
    memset(presetCache, 0, sizeof(presetCache));
    strcpy(cachedNames[0], "Panel");
    for(int i=1; i<=8; i++) sprintf(cachedNames[i], "Channel %d", i);
  }

  void setCallbacks(StateCallback cbState, NameCallback cbName) {
    onEffectChange = cbState;
    onNameChange = cbName;
  }
  
  void setActivePreset(int p) {
    if (p >= 0 && p <= 8) activePreset = p;
  }

  void loadFromCache(int presetNum) {
    if (presetNum < 0 || presetNum > 8) return;
    setActivePreset(presetNum);
    for(int i=0; i<8; i++) {
      bool savedState;
      if (i == 3) savedState = false; 
      else if (i == 0) savedState = false; 
      else savedState = presetCache[presetNum][i];
      
      effectsState[i] = savedState;
      if (onEffectChange) onEffectChange(i, savedState);
    }
  }

  void resetEffectsState() {
    for(int i=0; i<8; i++) {
      effectsState[i] = false;
      if (onEffectChange) onEffectChange(i, false);
    }
  }

  uint8_t checksum(const uint8_t* data, uint8_t len) {
    uint8_t sum = 0;
    for (uint8_t i = 0; i < len; i++) sum = (sum + data[i]) & 0x7F;
    return (128 - sum) & 0x7F;
  }

  void sendSysExChange(uint32_t addr, const uint8_t* data, uint8_t len) {
    uint8_t msg[64]; 
    msg[0] = SYSEX_START;
    memcpy(&msg[1], SYSEX_HEADER, 6);
    msg[7] = SYSEX_WRITE; 
    msg[8] = (addr >> 24) & 0x7F;
    msg[9] = (addr >> 16) & 0x7F;
    msg[10] = (addr >> 8) & 0x7F;
    msg[11] = addr & 0x7F;
    for(int i=0; i<len; i++) msg[12+i] = data[i];
    
    uint8_t* chkBuf = new uint8_t[4 + len];
    chkBuf[0] = msg[8]; chkBuf[1] = msg[9]; chkBuf[2] = msg[10]; chkBuf[3] = msg[11];
    memcpy(&chkBuf[4], data, len);
    
    msg[12+len] = checksum(chkBuf, 4+len);
    msg[13+len] = SYSEX_END;
    delete[] chkBuf;
    midi->SendSysEx(msg, 14+len);
  }

  void sendSysExRequest(uint32_t addr, uint32_t size) {
    uint8_t msg[20];
    msg[0] = SYSEX_START;
    memcpy(&msg[1], SYSEX_HEADER, 6);
    msg[7] = SYSEX_READ; 
    msg[8] = (addr >> 24) & 0x7F;
    msg[9] = (addr >> 16) & 0x7F;
    msg[10] = (addr >> 8) & 0x7F;
    msg[11] = addr & 0x7F;
    msg[12] = (size >> 24) & 0x7F;
    msg[13] = (size >> 16) & 0x7F;
    msg[14] = (size >> 8) & 0x7F;
    msg[15] = size & 0x7F;
    
    uint8_t tempForChk[] = {msg[8], msg[9], msg[10], msg[11], msg[12], msg[13], msg[14], msg[15]};
    msg[16] = checksum(tempForChk, 8);
    msg[17] = SYSEX_END;
    midi->SendSysEx(msg, 18);
  }

  void setEditorMode(bool active) {
     uint8_t data = active ? 0x01 : 0x00;
     sendSysExChange(P_EDIT, &data, 1);
  }

  void toggleEffect(uint8_t index) {
    if (index > 7) return; 
    
    bool newState = !effectsState[index];
    effectsState[index] = newState;
    
    // MUTE LOGIC (Index 3)
    if (index == 3) {
        uint8_t dataVal = newState ? 0 : 100;
        sendSysExChange(EN_TUNER, &dataVal, 1);
    } 
    // STANDARD EFFECTS (Including SOLO)
    else {
       uint8_t dataVal = newState ? 0x01 : 0x00;
       sendSysExChange(effectsAddr[index], &dataVal, 1);
    }

    if (onEffectChange) onEffectChange(index, newState);
  }

  void changePreset(uint8_t presetNum) { 
    if (presetNum < 1 || presetNum > 8) return;

    // Reset Solo State on preset change
    effectsState[0] = false;
    if (onEffectChange) onEffectChange(0, false);

    uint8_t targetAmpID = presetNum;

    uint8_t data[2] = {0x00, targetAmpID};
    sendSysExChange(PRESET, data, 2);
    
    setActivePreset(presetNum);
  }

  uint32_t getPresetBaseAddress(int p) {
    if (p == 0) return PANNEL_NAME;
    if (p == 1) return CH1_NAME;
    if (p == 2) return CH2_NAME;
    if (p == 3) return CH3_NAME;
    if (p == 4) return CH4_NAME;
    if (p == 5) return CH5_NAME;
    if (p == 6) return CH6_NAME;
    if (p == 7) return CH7_NAME;
    if (p == 8) return CH8_NAME;
    return PANNEL_NAME;
  }

  void requestNameByIndex(int idx) {
    sendSysExRequest(getPresetBaseAddress(idx), 0x10); 
  }

  void requestEffectByIndex(int idx) {
    if(idx != 0 && idx != 3 && idx < 8) {
      sendSysExRequest(effectsAddr[idx], 1);
    }
  }

  void requestEffectForPreset(int presetNum, int effIdx) {
    if(presetNum < 0 || presetNum > 8) return;
    if(effIdx == 0 || effIdx == 3 || effIdx > 7) return; // Skip SOLO and MUTE caching
    
    uint32_t base = getPresetBaseAddress(presetNum);
    uint32_t offset = effectsAddr[effIdx] & 0x0000FFFF;
    sendSysExRequest(base | offset, 1);
  }

  void requestVolume() {
    sendSysExRequest(ADDR_VOLUME, 1);
  }

  String getCachedName(int presetIndex) {
    if (presetIndex >= 0 && presetIndex <= 8) return String(cachedNames[presetIndex]);
    return "";
  }

  void processCompleteSysEx(uint8_t* msg, int len) {
    if (msg[0] == 0xF0 && msg[7] == SYSEX_WRITE) {
        uint32_t addr = 0;
        addr |= (msg[8] << 24);
        addr |= (msg[9] << 16);
        addr |= (msg[10] << 8);
        addr |= msg[11];
        
        if (len <= 16) { 
            uint8_t val = msg[12];
            for(int i=0; i<8; i++) {
              if (addr == effectsAddr[i]) {
                bool state;
                if (i == 3) {
                   state = (val == 0); 
                } else {
                   state = (val == 0x01);
                }
                
                if (effectsState[i] != state) {
                   effectsState[i] = state;
                   if (onEffectChange) onEffectChange(i, state);
                }
                if (isScanning && activePreset >= 0 && activePreset <= 8) {
                   presetCache[activePreset][i] = state;
                }
              }
            }

            // Check if it's a background cache read (0x10xxxxxx)
            if ((addr & 0xFF000000) == 0x10000000) {
               uint32_t base = addr & 0xFFFF0000;
               uint32_t offset = addr & 0x0000FFFF;
               
               int presetMatch = -1;
               if (base == PANNEL_NAME) presetMatch = 0;
               else if (base == CH1_NAME) presetMatch = 1;
               else if (base == CH2_NAME) presetMatch = 2;
               else if (base == CH3_NAME) presetMatch = 3;
               else if (base == CH4_NAME) presetMatch = 4;
               else if (base == CH5_NAME) presetMatch = 5;
               else if (base == CH6_NAME) presetMatch = 6;
               else if (base == CH7_NAME) presetMatch = 7;
               else if (base == CH8_NAME) presetMatch = 8;
               
               if (presetMatch != -1) {
                  for(int i=0; i<8; i++) {
                     if (offset == (effectsAddr[i] & 0x0000FFFF)) {
                        bool state = (val == 0x01);
                        presetCache[presetMatch][i] = state;
                     }
                  }
               }
            }
        }
        
        // Names
        if (len > 20) {
          int idx = -1;
          if (addr == PANNEL_NAME) idx = 0;
          else if (addr == CH1_NAME) idx = 1;
          else if (addr == CH2_NAME) idx = 2;
          else if (addr == CH3_NAME) idx = 3; 
          else if (addr == CH4_NAME) idx = 4; 
          else if (addr == CH5_NAME) idx = 5; 
          else if (addr == CH6_NAME) idx = 6; 
          else if (addr == CH7_NAME) idx = 7;
          else if (addr == CH8_NAME) idx = 8;

          if (idx != -1) {
              char tempBuf[17];
              memset(tempBuf, 0, 17);
              for(int k=0; k<16; k++) {
                 if (12+k < len - 2) tempBuf[k] = msg[12+k];
              }
              String s = String(tempBuf);
              s.trim();
              if (s.length() > 0) {
                s.toCharArray(cachedNames[idx], 17);
                if (onNameChange) onNameChange(s);
              }
          }
        }
    }
  }

  void parseIncoming() {
    uint8_t buf[64];
    uint16_t rcvd;
    while (midi->RecvData(&rcvd, buf) == 0 && rcvd > 0) {
      for (int i = 0; i < rcvd; i += 4) {
        if (i + 3 >= rcvd) break; 
        uint8_t cin = buf[i] & 0x0F;
        int count = 0;
        if (cin == 0x4) count = 3; 
        else if (cin == 0x5) count = 1; 
        else if (cin == 0x6) count = 2; 
        else if (cin == 0x7) count = 3; 
        for (int k = 1; k <= count; k++) {
          uint8_t byte = buf[i+k];
          if (byte == SYSEX_START) sysExIdx = 0; 
          if (sysExIdx < 128) sysExBuf[sysExIdx++] = byte;
          if (byte == SYSEX_END) {
             processCompleteSysEx(sysExBuf, sysExIdx);
             sysExIdx = 0; 
          }
        }
      }
    }
  }
};

#endif