#ifndef OLED_CONFIG_H
#define OLED_CONFIG_H

// ===================== OLEDs =====================
// Multiplexer Default Wiring
static const int SDA_MUX = 8;    
static const int SCL_MUX = 9;

// Second I2C bus for SH1107
static const int SDA2 = 16;   
static const int SCL2 = 15;

static const int SH1107_RST = -1;

#define OLED_W_128x32 128
#define OLED_H_128x32 32
#define OLED_W_128x128 128
#define OLED_H_128x128 128

// Addresses
#define OLED_ADDR 0x3C   // for the SSD1306s
#define TCA_ADDR 0x70    // TCA9548A default

#endif
