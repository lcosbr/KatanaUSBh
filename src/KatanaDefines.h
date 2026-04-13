#ifndef KATANA_DEFINES_H
#define KATANA_DEFINES_H

#include <Arduino.h>

const byte SYSEX_WRITE = 0x12; 
const byte SYSEX_READ  = 0x11; 
const byte SYSEX_START = 0xF0;
const byte SYSEX_HEADER[6] = {0x41, 0x00, 0x00, 0x00, 0x00, 0x33}; 
const byte SYSEX_END   = 0xF7;

// --- Addresses ---
const unsigned long P_EDIT     = 0x7F000001; 
const unsigned long PRESET     = 0x00010000; 

// Effects
const unsigned long EN_BOOSTER = 0x60000010; 
const unsigned long EN_MOD     = 0x60000100; 
const unsigned long EN_FX      = 0x60000300; 
const unsigned long EN_DELAY   = 0x60000500; 
const unsigned long EN_REVERB  = 0x60000540; 
const unsigned long EN_DELAY2  = 0x60000520; 

// Solo & Volume
const unsigned long EN_SOLO     = 0x60000614; 
const unsigned long ADDR_VOLUME = 0x60000028; 

// Mute via Foot Volume
const unsigned long EN_TUNER    = 0x60000561; 

// Names
const unsigned long PANNEL_NAME = 0x10000000;

// Bank A
const unsigned long CH1_NAME    = 0x10010000; 
const unsigned long CH2_NAME    = 0x10020000; 
const unsigned long CH3_NAME    = 0x10030000; 
const unsigned long CH4_NAME    = 0x10040000; 

// Bank B
const unsigned long CH5_NAME    = 0x10050000; 
const unsigned long CH6_NAME    = 0x10060000; 
const unsigned long CH7_NAME    = 0x10070000; 
const unsigned long CH8_NAME    = 0x10080000; 

#endif
