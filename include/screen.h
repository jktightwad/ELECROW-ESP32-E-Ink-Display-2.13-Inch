#pragma once
#include <Adafruit_GFX.h>

// Screen layouts, drawn on a landscape canvas (GFXcanvas1 in rotation 1:
// 250 x 128, of which y = 0..121 is visible). Colour 1 = black.

// "Tell David:" heading, then `line` word-wrapped and fitted below it.
// Returns false if the line had to be clipped.
bool drawDavidScreen(Adafruit_GFX& gfx, const char* line);

// `text` word-wrapped and fitted to the whole screen.
bool drawTextScreen(Adafruit_GFX& gfx, const char* text);
