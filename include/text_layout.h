#pragma once
#include <Adafruit_GFX.h>

// Converts UTF-8 text to what the built-in fonts can draw (printable ASCII):
// curly quotes -> straight, ellipsis -> "...", dashes -> "-", other
// non-ASCII characters are dropped.
String toFontText(const char* utf8);

// Draws `text` word-wrapped and centred inside the rectangle (x, y, w, h) of
// `gfx`, using the largest of the built-in fonts whose wrapped text fits.
// Colour 1 = black. `text` may be UTF-8; it is passed through toFontText().
// Words too wide for a line are split. Returns false if even the smallest font
// could not fit the text; it is then cut short and ends with "...".
bool drawWrappedText(Adafruit_GFX& gfx, const char* text,
                     int16_t x, int16_t y, int16_t w, int16_t h);
