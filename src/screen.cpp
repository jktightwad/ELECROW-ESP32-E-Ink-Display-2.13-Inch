#include "screen.h"
#include <Fonts/FreeSansBold9pt7b.h>
#include "text_layout.h"

namespace {
const int16_t SCREEN_W = 250;
const int16_t SCREEN_H = 122;    // visible rows
const int16_t MARGIN = 5;
const char* const HEADING = "Tell David:";
}

bool drawDavidScreen(Adafruit_GFX& gfx, const char* line) {
  gfx.fillScreen(0);

  // Heading: top-left, with a rule under it.
  gfx.setFont(&FreeSansBold9pt7b);
  gfx.setTextColor(1);
  gfx.setTextWrap(false);
  int16_t x1, y1;
  uint16_t w, h;
  gfx.getTextBounds(HEADING, 0, 0, &x1, &y1, &w, &h);
  gfx.setCursor(MARGIN - x1, MARGIN - y1);
  gfx.print(HEADING);
  int16_t rule_y = MARGIN + h + 3;
  gfx.drawFastHLine(MARGIN, rule_y, SCREEN_W - 2 * MARGIN, 1);

  int16_t body_top = rule_y + 4;
  return drawWrappedText(gfx, line, MARGIN, body_top,
                         SCREEN_W - 2 * MARGIN, SCREEN_H - MARGIN - body_top);
}

bool drawTextScreen(Adafruit_GFX& gfx, const char* text) {
  gfx.fillScreen(0);
  return drawWrappedText(gfx, text, MARGIN, MARGIN, SCREEN_W - 2 * MARGIN, SCREEN_H - 2 * MARGIN);
}
