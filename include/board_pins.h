#pragma once
// Pin map for the CrowPanel ESP32 E-Paper 2.13" (Elecrow part DIE01021S).
//
// NOT VERIFIED. Check every value against Elecrow's schematic
// "CrowPanel_ESP32_Display-2.13(E)_Inch.pdf" or their 2.13" Arduino example
// before trusting it.
//
// Sources for the current values:
// - EPD_POWER = 7: Elecrow's 2.9" and 4.2" CrowPanel examples set GPIO7 HIGH
//   before initialising the panel. The 2.13" schematic text also shows GPIO7 near
//   a VCC net. This value is the most likely of the set.
// - The SPI pins below are defaults from a third-party ESP-IDF component
//   (antunesls/crowpanel_epaper_driver_component). That same list puts BUSY on
//   GPIO7, which conflicts with the power pin above, so treat these as placeholders.

#define EPD_POWER 7

#define EPD_BUSY  -1   // TODO: confirm from schematic
#define EPD_RST   -1   // TODO
#define EPD_DC    -1   // TODO
#define EPD_CS    -1   // TODO
#define EPD_SCK   -1   // TODO
#define EPD_MOSI  -1   // TODO

#define PINS_VERIFIED 0   // set to 1 once the values above are confirmed
