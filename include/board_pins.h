#pragma once
// Pin map for the CrowPanel ESP32 E-Paper 2.13" (Elecrow part DIE01021S).
//
// Source: Elecrow's official example code,
// github.com/Elecrow-RD/CrowPanel-ESP32-2.13-E-paper-HMI-Display-with-122-250
//   - display pins:  example/arduino/Example Code/2.13_Global_refresh/spi.h
//   - power pin:     2.13_Global_refresh.ino  (pinMode(7, OUTPUT); digitalWrite(7, HIGH))
//   - buttons:       example/arduino/Example Code/2.13_key/2.13_key.ino
// These connections are internal to the board; they are fixed, not wiring choices.

// E-paper power rail: must be HIGH before the panel responds.
#define EPD_POWER 7

// E-paper SPI + control lines
#define EPD_SCK   12
#define EPD_MOSI  11
#define EPD_RST   10
#define EPD_DC    13
#define EPD_CS    14
#define EPD_BUSY   9

// Buttons (Elecrow's example reads them as LOW when pressed)
#define KEY_HOME   2   // menu button
#define KEY_EXIT   1   // back button
#define KEY_PREV   6   // dial up
#define KEY_NEXT   4   // dial down
#define KEY_OK     5   // dial press
