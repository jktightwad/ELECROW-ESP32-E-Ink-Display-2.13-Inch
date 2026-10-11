#pragma once
#include <Arduino.h>
#include <vector>

// The list of lines to pick from. Loaded from /lines.txt on the board's
// file system (LittleFS), falling back to the built-in list in lines.h.
// Format: plain UTF-8 text, one entry per line; blank lines are ignored.
class LineStore {
 public:
  void begin();
  size_t size() const { return lines_.size(); }
  const String& get(size_t i) const { return lines_[i]; }

  // Appends one line and saves. Returns false if it is empty or already present.
  bool add(const String& line);

  // The list as newline-separated text (the same format as /lines.txt).
  String toText() const;

 private:
  static std::vector<String> parse(const String& text);
  void save() const;
  std::vector<String> lines_;
};
