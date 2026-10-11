#pragma once
#include <Arduino.h>

// Commands arrive as JSON on the shared Adafruit IO feed, in the same style as
// the On Air Light fleet:
//   {"deviceId":"nodavid","cmd":"random"}
//   {"deviceId":"nodavid","cmd":"show","text":"Back in 5"}
//   {"deviceId":"nodavid","cmd":"add","text":"A new line"}
//   {"deviceId":"nodavid","cmd":"location","text":"Albany, NY"}
// "deviceIds":["nodavid", ...] also works. A message whose JSON is wrapped in a
// "value" string (as the On Air Light firmware also accepts) is unwrapped first.
// Messages for other devices, or that aren't JSON, are ignored silently.
enum class CommandType { None, Random, Show, Add, Location };

struct Command {
  CommandType type = CommandType::None;
  String text;
  String error;   // set when the message targets this device but is malformed
};

Command parseCommand(const char* payload, size_t len, const char* device_id);
