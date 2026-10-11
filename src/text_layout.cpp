#include "text_layout.h"
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>

namespace {

const GFXfont* const FONTS[] = {&FreeSans18pt7b, &FreeSans12pt7b, &FreeSans9pt7b};  // largest first
const int NUM_FONTS = sizeof(FONTS) / sizeof(FONTS[0]);
const int MAX_LINES = 12;

struct Bounds { int16_t x1, y1; uint16_t w, h; };

// Ink bounds of `s` drawn with its baseline at y = 0 (y1 is negative: above the baseline).
Bounds measure(Adafruit_GFX& gfx, const String& s) {
  Bounds b;
  gfx.getTextBounds(s.c_str(), 0, 0, &b.x1, &b.y1, &b.w, &b.h);
  return b;
}

// Greedy word wrap. Words wider than `width` are split across lines.
// Returns the number of lines, or -1 if more than MAX_LINES are needed.
int wrap(Adafruit_GFX& gfx, const String& text, int16_t width, String out[MAX_LINES]) {
  int n = 0;
  String line, word;
  auto push = [&](const String& l) {
    if (n >= MAX_LINES) return false;
    out[n++] = l;
    return true;
  };
  String remaining = text + ' ';
  for (size_t i = 0; i < remaining.length(); i++) {
    char c = remaining[i];
    if (c != ' ' && c != '\n') { word += c; continue; }
    if (word.length()) {
      String candidate = line.length() ? line + ' ' + word : word;
      if (measure(gfx, candidate).w <= width) {
        line = candidate;
      } else {
        if (line.length() && !push(line)) return -1;
        line = "";
        // Split a word that is too wide on its own.
        while (measure(gfx, word).w > width) {
          size_t k = 1;
          while (k < word.length() && measure(gfx, word.substring(0, k + 1)).w <= width) k++;
          if (!push(word.substring(0, k))) return -1;
          word = word.substring(k);
        }
        line = word;
      }
      word = "";
    }
    if (c == '\n' && line.length()) {
      if (!push(line)) return -1;
      line = "";
    }
  }
  if (line.length() && !push(line)) return -1;
  return n;
}

// Height of the inked block for `n` lines spaced `line_h` apart, and the
// offset from the block's top to the first baseline.
void blockExtent(Adafruit_GFX& gfx, const String lines[], int n, int16_t line_h,
                 int16_t* height, int16_t* first_baseline) {
  int16_t top = 0, bottom = 0;
  for (int i = 0; i < n; i++) {
    Bounds b = measure(gfx, lines[i]);
    int16_t t = i * line_h + b.y1, btm = i * line_h + b.y1 + b.h;
    if (i == 0 || t < top) top = t;
    if (i == 0 || btm > bottom) bottom = btm;
  }
  *height = bottom - top;
  *first_baseline = -top;
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

bool drawWrappedText(Adafruit_GFX& gfx, const char* text,
                     int16_t x, int16_t y, int16_t w, int16_t h) {
  String clean = toFontText(text);
  String lines[MAX_LINES];
  int n = 0;
  int16_t block_h = 0, first_baseline = 0;
  bool fits = false;
  const GFXfont* font = FONTS[NUM_FONTS - 1];

  for (int f = 0; f < NUM_FONTS && !fits; f++) {
    gfx.setFont(FONTS[f]);
    n = wrap(gfx, clean, w, lines);
    if (n <= 0) continue;
    blockExtent(gfx, lines, n, FONTS[f]->yAdvance, &block_h, &first_baseline);
    if (block_h <= h) { fits = true; font = FONTS[f]; }
  }
  if (!fits) {
    // Smallest font: keep as many lines as fit and end the last with "...".
    gfx.setFont(font);
    n = wrap(gfx, clean, w, lines);
    if (n < 0) n = MAX_LINES;
    blockExtent(gfx, lines, n, font->yAdvance, &block_h, &first_baseline);
    while (n > 1 && block_h > h) {
      n--;
      blockExtent(gfx, lines, n, font->yAdvance, &block_h, &first_baseline);
    }
    String& last = lines[n - 1];
    while (last.length() && measure(gfx, last + "...").w > w) last.remove(last.length() - 1);
    last.trim();
    last += "...";
  }

  gfx.setFont(font);
  if (fits && n > 1) {
    // Balance the lines: narrow the wrap width as far as possible without
    // adding a line, so the last line isn't left with a single short word.
    String trial[MAX_LINES];
    for (int16_t bw = w - 2; bw > w / 3; bw -= 2) {
      if (wrap(gfx, clean, bw, trial) != n) break;
      for (int i = 0; i < n; i++) lines[i] = trial[i];
    }
  }

  gfx.setTextColor(1);
  gfx.setTextWrap(false);
  int16_t top = y + (h - block_h) / 2;
  if (top < y) top = y;
  int16_t baseline = top + first_baseline;
  for (int i = 0; i < n; i++) {
    Bounds b = measure(gfx, lines[i]);
    gfx.setCursor(x + (w - (int16_t)b.w) / 2 - b.x1, baseline);
    gfx.print(lines[i]);
    baseline += font->yAdvance;
  }
  return fits;
}
