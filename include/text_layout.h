#pragma once
#include <Adafruit_GFX.h>

// Draws `text` word-wrapped and centred inside the rectangle (x, y, w, h) of
// `gfx`, using the largest of the built-in fonts that fits. Colour 1 = black.
void drawWrappedText(Adafruit_GFX& gfx, const char* text,
                     int16_t x, int16_t y, int16_t w, int16_t h);
