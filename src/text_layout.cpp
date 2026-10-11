#include "text_layout.h"
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>

namespace {

const GFXfont* const FONTS[] = {&FreeSans18pt7b, &FreeSans12pt7b, &FreeSans9pt7b};  // largest first
const int MAX_LINES = 8;

int16_t textWidth(Adafruit_GFX& gfx, const String& s) {
  int16_t x1, y1;
  uint16_t w, h;
  gfx.getTextBounds(s.c_str(), 0, 0, &x1, &y1, &w, &h);
  return w;
}

// Greedy word wrap. Returns the number of lines, or -1 if a single word is
// wider than `width` or more than MAX_LINES are needed.
int wrap(Adafruit_GFX& gfx, const char* text, int16_t width, String out[MAX_LINES]) {
  int n = 0;
  String line, word;
  String remaining(text);
  remaining += ' ';
  for (size_t i = 0; i < remaining.length(); i++) {
    char c = remaining[i];
    if (c != ' ' && c != '\n') { word += c; continue; }
    if (word.length()) {
      String candidate = line.length() ? line + ' ' + word : word;
      if (textWidth(gfx, candidate) <= width) {
        line = candidate;
      } else {
        if (textWidth(gfx, word) > width) return -1;
        if (n >= MAX_LINES) return -1;
        out[n++] = line;
        line = word;
      }
      word = "";
    }
    if (c == '\n' && line.length()) {
      if (n >= MAX_LINES) return -1;
      out[n++] = line;
      line = "";
    }
  }
  if (line.length()) {
    if (n >= MAX_LINES) return -1;
    out[n++] = line;
  }
  return n;
}

}  // namespace

String toFontText(const char* utf8) {
  String out;
  const uint8_t* p = (const uint8_t*)utf8;
  while (*p) {
    uint8_t c = *p;
    if (c < 0x80) {
      if (c == '\n' || (c >= 0x20 && c < 0x7F)) out += (char)c;
      else if (c == '\t') out += ' ';
      p++;
      continue;
    }
    // Multi-byte UTF-8 sequence: length from the lead byte.
    int len = (c >= 0xF0) ? 4 : (c >= 0xE0) ? 3 : (c >= 0xC0) ? 2 : 1;
    if (len == 3 && p[0] == 0xE2 && p[1] == 0x80) {
      switch (p[2]) {
        case 0x98: case 0x99: out += '\''; break;      // ‘ ’
        case 0x9C: case 0x9D: out += '"'; break;       // “ ”
        case 0xA6: out += "..."; break;                // …
        case 0x93: case 0x94: out += '-'; break;       // – —
      }
    } else if (len == 2 && p[0] == 0xC2 && p[1] == 0xA0) {
      out += ' ';                                      // non-breaking space
    }
    for (int i = 0; i < len && *p; i++) p++;
  }
  return out;
}

void drawWrappedText(Adafruit_GFX& gfx, const char* text,
                     int16_t x, int16_t y, int16_t w, int16_t h) {
  String clean = toFontText(text);
  text = clean.c_str();
  String lines[MAX_LINES];
  const GFXfont* font = FONTS[sizeof(FONTS) / sizeof(FONTS[0]) - 1];
  int n = -1;
  for (const GFXfont* f : FONTS) {
    gfx.setFont(f);
    n = wrap(gfx, text, w, lines);
    if (n > 0 && n * f->yAdvance <= h) { font = f; break; }
    n = -1;
  }
  gfx.setFont(font);
  if (n < 0) n = wrap(gfx, text, w, lines);   // smallest font, even if it overflows
  if (n < 0) n = 0;

  gfx.setTextColor(1);
  gfx.setTextWrap(false);
  const int16_t lineH = font->yAdvance;
  const int16_t ascent = lineH * 3 / 4;       // approximate baseline offset
  int16_t baseline = y + (h - n * lineH) / 2 + ascent;
  for (int i = 0; i < n; i++) {
    gfx.setCursor(x + (w - textWidth(gfx, lines[i])) / 2, baseline);
    gfx.print(lines[i]);
    baseline += lineH;
  }
}
