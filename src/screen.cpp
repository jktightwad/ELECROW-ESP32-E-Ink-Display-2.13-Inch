#include "screen.h"
#include <Fonts/FreeSansBold9pt7b.h>
#include "text_layout.h"
#include "weather.h"
#include "fonts/Barlow_SB13.h"
#include "fonts/Barlow_SB14.h"
#include "fonts/Barlow_M12.h"
#include "fonts/BarlowCond_SB50.h"
#include "fonts/WeatherIcons_L.h"
#include "fonts/WeatherIcons_M.h"
#include "fonts/WeatherIcons_S.h"
#include "fonts/weather_icons.h"

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

namespace {

// Ink width of `s` in the current font.
int16_t inkWidth(Adafruit_GFX& gfx, const char* s) {
  int16_t x1, y1;
  uint16_t w, h;
  gfx.getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
  return w;
}

// Draws `s` with the top of the font's digits at `top`. align: 0 left, 1 centre, 2 right of x.
void drawText(Adafruit_GFX& gfx, const GFXfont* font, int16_t x, int16_t top, const char* s,
              int align = 0, uint16_t color = 1) {
  gfx.setFont(font);
  gfx.setTextColor(color);
  int16_t x1, y1;
  uint16_t w, h;
  gfx.getTextBounds("0", 0, 0, &x1, &y1, &w, &h);   // digit height = reference cap height
  int16_t baseline = top - y1;
  gfx.getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
  int16_t cx = (align == 0) ? x - x1 : (align == 1) ? x - (int16_t)w / 2 - x1 : x - (int16_t)w - x1;
  gfx.setCursor(cx, baseline);
  gfx.print(s);
}

// Draws one icon glyph centred in the box (x, y, w, h).
void drawIcon(Adafruit_GFX& gfx, const GFXfont* font, char icon, int16_t x, int16_t y,
              int16_t w, int16_t h) {
  char s[2] = {icon, 0};
  gfx.setFont(font);
  gfx.setTextColor(1);
  int16_t x1, y1;
  uint16_t bw, bh;
  gfx.getTextBounds(s, 0, 0, &x1, &y1, &bw, &bh);
  gfx.setCursor(x + (w - (int16_t)bw) / 2 - x1, y + (h - (int16_t)bh) / 2 - y1);
  gfx.print(s);
}

const char* const WEEKDAYS[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char* const MONTHS[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                              "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

}  // namespace

String formatTime12(int hour, int minute) {
  char buf[12];
  int h12 = hour % 12 == 0 ? 12 : hour % 12;
  snprintf(buf, sizeof buf, "%d:%02d %s", h12, minute, hour < 12 ? "AM" : "PM");
  return String(buf);
}

void drawDashboardScreen(Adafruit_GFX& gfx, const DashboardInfo& info) {
  gfx.fillScreen(0);
  gfx.setTextWrap(false);
  const WeatherData* w = (info.weather && info.weather->valid) ? info.weather : nullptr;
  char buf[32];

  // Top bar: location | date | time (white on black).
  gfx.fillRect(0, 0, SCREEN_W, 16, 1);
  // Location, shortened with "..." if it would run into the date.
  gfx.setFont(&Barlow_SB13);
  String loc = info.location;
  if (inkWidth(gfx, loc.c_str()) > 96) {
    while (loc.length() > 1 && inkWidth(gfx, (loc + "...").c_str()) > 96) loc.remove(loc.length() - 1);
    loc.trim();
    loc += "...";
  }
  drawText(gfx, &Barlow_SB13, 4, 3, loc.c_str(), 0, 0);
  if (info.have_time) {
    snprintf(buf, sizeof buf, "%s %s %d", WEEKDAYS[info.weekday % 7], MONTHS[(info.month + 11) % 12], info.day);
    drawText(gfx, &Barlow_SB13, 152, 3, buf, 1, 0);
    drawText(gfx, &Barlow_SB13, SCREEN_W - 4, 3, formatTime12(info.hour, info.minute).c_str(), 2, 0);
  } else {
    drawText(gfx, &Barlow_SB13, SCREEN_W - 4, 3, "--:--", 2, 0);
  }

  // Condition icon, temperature, feels-like, high/low.
  drawIcon(gfx, &WeatherIcons_L, w ? weatherIcon(w->code, w->is_day) : WI::NA, 2, 18, 66, 76);
  if (w) snprintf(buf, sizeof buf, "%d~", (int)lroundf(w->temp));
  else snprintf(buf, sizeof buf, "--");
  drawText(gfx, &BarlowCond_SB50, 72, 22, buf);
  if (w) {
    snprintf(buf, sizeof buf, "Feels %d~", (int)lroundf(w->feels));
    drawText(gfx, &Barlow_M12, 74, 69, buf);
    snprintf(buf, sizeof buf, "H %d~  L %d~", (int)lroundf(w->high), (int)lroundf(w->low));
    drawText(gfx, &Barlow_M12, 74, 83, buf);
  }

  // Right column: humidity, wind, chance of precipitation.
  gfx.drawFastVLine(148, 22, 64, 1);
  const char icons[3] = {WI::HUMIDITY, WI::WIND, WI::RAINDROP};
  String vals[3] = {"--", "--", "--"};
  if (w) {
    vals[0] = String(w->humidity) + "%";
    vals[1] = String((int)lroundf(w->wind)) + " mph " + compass(w->wind_dir);
    gfx.setFont(&Barlow_SB14);
    if (inkWidth(gfx, vals[1].c_str()) > SCREEN_W - 2 - 174) vals[1] = String((int)lroundf(w->wind)) + " mph";
    vals[2] = String(w->precip_prob) + "%";
  }
  for (int i = 0; i < 3; i++) {
    int16_t top = 24 + i * 21;
    drawIcon(gfx, &WeatherIcons_S, icons[i], 151, top - 3, 20, 18);
    drawText(gfx, &Barlow_SB14, 174, top + 1, vals[i].c_str());
  }

  // Bottom row: sunrise, sunset, condition (or "offline").
  gfx.drawFastHLine(4, 97, SCREEN_W - 8, 1);
  const int16_t row_top = 104;
  drawIcon(gfx, &WeatherIcons_M, WI::SUNRISE, 2, 98, 24, 22);
  drawIcon(gfx, &WeatherIcons_M, WI::SUNSET, 82, 98, 24, 22);
  if (w && w->sunrise[0]) {
    drawText(gfx, &Barlow_SB13, 27, row_top, formatTime12(atoi(w->sunrise), atoi(w->sunrise + 3)).c_str());
    drawText(gfx, &Barlow_SB13, 107, row_top, formatTime12(atoi(w->sunset), atoi(w->sunset + 3)).c_str());
  }
  const char* cond = info.stale ? "offline" : (w ? weatherLabel(w->code) : "loading...");
  drawText(gfx, &Barlow_SB13, SCREEN_W - 4, row_top, cond, 2);
}

void drawBootScreen(Adafruit_GFX& gfx, const BootStep* steps, int count) {
  gfx.fillScreen(0);
  gfx.fillRect(0, 0, SCREEN_W, 16, 1);
  drawText(gfx, &Barlow_SB13, 4, 3, "POWERING ON", 0, 0);
  const int16_t row_h = 20, box = 10;
  for (int i = 0; i < count; i++) {
    const BootStep& st = steps[i];
    int16_t top = 23 + i * row_h;
    int16_t bx = 6, by = top;
    // Status box: done = filled with a check, failed = X, active = dot, pending/skipped = empty.
    switch (st.state) {
      case StepState::Done:
        gfx.fillRect(bx, by, box, box, 1);
        gfx.drawLine(bx + 2, by + 5, bx + 4, by + 7, 0);
        gfx.drawLine(bx + 4, by + 7, bx + 8, by + 2, 0);
        gfx.drawLine(bx + 2, by + 4, bx + 4, by + 6, 0);
        gfx.drawLine(bx + 4, by + 6, bx + 8, by + 1, 0);
        break;
      case StepState::Failed:
        gfx.drawRect(bx, by, box, box, 1);
        gfx.drawLine(bx + 2, by + 2, bx + 7, by + 7, 1);
        gfx.drawLine(bx + 7, by + 2, bx + 2, by + 7, 1);
        break;
      case StepState::Active:
        gfx.drawRect(bx, by, box, box, 1);
        gfx.fillRect(bx + 3, by + 3, 4, 4, 1);
        break;
      default:
        gfx.drawRect(bx, by, box, box, 1);
        break;
    }
    String text = st.label;
    if (st.state == StepState::Active) text += "...";
    else if (st.detail.length()) text += String(": ") + st.detail;
    // Keep within the screen width.
    gfx.setFont(&Barlow_SB13);
    if (inkWidth(gfx, text.c_str()) > SCREEN_W - 26) {
      while (text.length() > 1 && inkWidth(gfx, (text + "...").c_str()) > SCREEN_W - 26) text.remove(text.length() - 1);
      text += "...";
    }
    drawText(gfx, st.state == StepState::Pending ? &Barlow_M12 : &Barlow_SB13, 22, top, text.c_str());
  }
}

void drawStatusScreen(Adafruit_GFX& gfx, const char* title, const StatusRow* rows, int count) {
  gfx.fillScreen(0);
  gfx.fillRect(0, 0, SCREEN_W, 16, 1);
  drawText(gfx, &Barlow_SB13, 4, 3, "STATUS", 0, 0);
  drawText(gfx, &Barlow_SB13, SCREEN_W - 4, 3, title, 2, 0);
  const int16_t row_h = 17, value_x = 66;
  for (int i = 0; i < count; i++) {
    int16_t top = 21 + i * row_h;
    drawText(gfx, &Barlow_M12, 4, top + 1, rows[i].label);
    String v = rows[i].value;
    gfx.setFont(&Barlow_SB13);
    if (inkWidth(gfx, v.c_str()) > SCREEN_W - 4 - value_x) {
      while (v.length() > 1 && inkWidth(gfx, (v + "...").c_str()) > SCREEN_W - 4 - value_x) v.remove(v.length() - 1);
      v += "...";
    }
    drawText(gfx, &Barlow_SB13, value_x, top, v.c_str());
  }
}

void drawSetupScreen(Adafruit_GFX& gfx, const char* ap_name) {
  gfx.fillScreen(0);
  gfx.fillRect(0, 0, SCREEN_W, 16, 1);
  drawText(gfx, &Barlow_SB13, 4, 3, "WI-FI SETUP", 0, 0);
  drawText(gfx, &Barlow_SB13, 6, 26, "1. On your phone, join Wi-Fi:");
  drawText(gfx, &Barlow_SB14, 22, 44, ap_name);
  drawText(gfx, &Barlow_SB13, 6, 64, "2. Pick your network on the page");
  drawText(gfx, &Barlow_SB13, 22, 80, "that opens (or 192.168.4.1)");
  drawText(gfx, &Barlow_M12, 6, 102, "Closes after 5 minutes if unused.");
}

bool drawTextScreen(Adafruit_GFX& gfx, const char* text) {
  gfx.fillScreen(0);
  return drawWrappedText(gfx, text, MARGIN, MARGIN, SCREEN_W - 2 * MARGIN, SCREEN_H - 2 * MARGIN);
}
