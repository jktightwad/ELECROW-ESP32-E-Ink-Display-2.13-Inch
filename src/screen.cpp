#include "screen.h"
#include <Fonts/FreeSans9pt7b.h>
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

void drawDashboardScreen(Adafruit_GFX& gfx, const DashboardInfo& info) {
  gfx.fillScreen(0);
  gfx.setTextColor(1);
  gfx.setTextWrap(false);

  int16_t x1, y1;
  uint16_t w, h;
  gfx.setFont(&FreeSansBold9pt7b);
  gfx.getTextBounds("Dashboard", 0, 0, &x1, &y1, &w, &h);
  gfx.setCursor(MARGIN - x1, MARGIN - y1);
  gfx.print("Dashboard");
  int16_t rule_y = MARGIN + h + 3;
  gfx.drawFastHLine(MARGIN, rule_y, SCREEN_W - 2 * MARGIN, 1);

  gfx.setFont(&FreeSans9pt7b);
  const int16_t line_h = 19;
  int16_t baseline = rule_y + 4 + 14;
  gfx.setCursor(MARGIN, baseline);
  gfx.print(String("Wi-Fi: ") + info.wifi);
  baseline += line_h;
  gfx.setCursor(MARGIN, baseline);
  gfx.print(String("MQTT: ") + info.mqtt);
  baseline += line_h;
  if (info.ip && *info.ip) {
    gfx.setCursor(MARGIN, baseline);
    gfx.print(String("IP: ") + info.ip);
  }

  // Footer: list size bottom-left.
  gfx.setCursor(MARGIN, SCREEN_H - MARGIN - 4);
  gfx.print(String((unsigned long)info.line_count) + " lines");

  // Footer hint, bottom-right, inverted.
  const char* hint = "Dial: Tell David";
  gfx.getTextBounds(hint, 0, 0, &x1, &y1, &w, &h);
  int16_t box_h = h + 6, box_w = w + 10;
  int16_t box_x = SCREEN_W - MARGIN - box_w, box_y = SCREEN_H - MARGIN - box_h;
  gfx.fillRect(box_x, box_y, box_w, box_h, 1);
  gfx.setTextColor(0);
  gfx.setCursor(box_x + 5 - x1, box_y + 3 - y1);
  gfx.print(hint);
  gfx.setTextColor(1);
}

bool drawTextScreen(Adafruit_GFX& gfx, const char* text) {
  gfx.fillScreen(0);
  return drawWrappedText(gfx, text, MARGIN, MARGIN, SCREEN_W - 2 * MARGIN, SCREEN_H - 2 * MARGIN);
}
