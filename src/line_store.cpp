#include "line_store.h"
#include <LittleFS.h>
#include "lines.h"

namespace {
const char* const PATH = "/lines.txt";
bool fs_ok = false;
}

std::vector<String> LineStore::parse(const String& text) {
  std::vector<String> out;
  int start = 0;
  while (start <= (int)text.length()) {
    int end = text.indexOf('\n', start);
    if (end < 0) end = text.length();
    String line = text.substring(start, end);
    line.trim();                         // also removes a trailing '\r' (Windows line endings)
    if (line.length()) out.push_back(line);
    start = end + 1;
  }
  return out;
}

void LineStore::begin() {
  fs_ok = LittleFS.begin(true);          // true: format the partition if it can't be mounted
  if (fs_ok && LittleFS.exists(PATH)) {
    File f = LittleFS.open(PATH, "r");
    lines_ = parse(f.readString());
    f.close();
  }
  if (lines_.empty()) {
    for (size_t i = 0; i < DEFAULT_LINE_COUNT; i++) lines_.push_back(DEFAULT_LINES[i]);
    Serial.printf("Lines: %u built-in (no %s on file system)\n", (unsigned)lines_.size(), PATH);
  } else {
    Serial.printf("Lines: %u loaded from %s\n", (unsigned)lines_.size(), PATH);
  }
}

String LineStore::toText() const {
  String s;
  for (const String& l : lines_) { s += l; s += '\n'; }
  return s;
}

void LineStore::save() const {
  if (!fs_ok) return;
  File f = LittleFS.open(PATH, "w");
  if (!f) { Serial.println("Lines: could not write file"); return; }
  f.print(toText());
  f.close();
}

bool LineStore::replaceAll(const String& text) {
  std::vector<String> parsed = parse(text);
  if (parsed.empty() || parsed == lines_) return false;
  lines_ = parsed;
  save();
  return true;
}

bool LineStore::add(const String& line) {
  String l = line;
  l.trim();
  if (!l.length()) return false;
  for (const String& existing : lines_) if (existing == l) return false;
  lines_.push_back(l);
  save();
  return true;
}
