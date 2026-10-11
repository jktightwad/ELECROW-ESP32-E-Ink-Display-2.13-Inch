#pragma once
#include <Adafruit_GFX.h>

// Draws `text` word-wrapped and centred inside the rectangle (x, y, w, h) of
// `gfx`, using the largest of the built-in fonts that fits. Colour 1 = black.
// Converts UTF-8 text to what the built-in fonts can draw (printable ASCII):
// curly quotes -> straight, ellipsis -> "...", dashes -> "-", other
// non-ASCII characters are dropped.
String toFontText(const char* utf8);

// `text` may be UTF-8; it is passed through toFontText() first.
void drawWrappedText(Adafruit_GFX& gfx, const char* text,
                     int16_t x, int16_t y, int16_t w, int16_t h);
