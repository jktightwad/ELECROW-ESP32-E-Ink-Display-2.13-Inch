#include "weather.h"
#include "fonts/weather_icons.h"

// Codes per Open-Meteo's WMO weather interpretation code table.
const char* weatherLabel(int code) {
  switch (code) {
    case 0:  return "Clear";
    case 1:  return "Mostly clear";
    case 2:  return "Partly cloudy";
    case 3:  return "Overcast";
    case 45: return "Fog";
    case 48: return "Rime fog";
    case 51: return "Light drizzle";
    case 53: return "Drizzle";
    case 55: return "Heavy drizzle";
    case 56: case 57: return "Frz drizzle";
    case 61: return "Light rain";
    case 63: return "Rain";
    case 65: return "Heavy rain";
    case 66: case 67: return "Freezing rain";
    case 71: return "Light snow";
    case 73: return "Snow";
    case 75: return "Heavy snow";
    case 77: return "Snow grains";
    case 80: return "Light showers";
    case 81: return "Showers";
    case 82: return "Heavy showers";
    case 85: case 86: return "Snow showers";
    case 95: return "Thunderstorm";
    case 96: case 99: return "T-storm, hail";
    default: return "--";
  }
}

char weatherIcon(int code, bool day) {
  switch (code) {
    case 0:  return day ? WI::DAY_SUNNY : WI::NIGHT_CLEAR;
    case 1:  return day ? WI::DAY_SUNNY_OVERCAST : WI::NIGHT_CLOUDY;
    case 2:  return day ? WI::DAY_CLOUDY : WI::NIGHT_CLOUDY;
    case 3:  return WI::CLOUDY;
    case 45: case 48: return WI::FOG;
    case 51: case 53: case 55: return WI::SPRINKLE;
    case 56: case 57: case 66: case 67: return WI::RAIN_MIX;
    case 61: case 63: return day ? WI::DAY_RAIN : WI::NIGHT_RAIN;
    case 65: return WI::RAIN;
    case 71: case 73: case 77: return day ? WI::DAY_SNOW : WI::NIGHT_SNOW;
    case 75: case 85: case 86: return WI::SNOW;
    case 80: case 81: case 82: return day ? WI::DAY_SHOWERS : WI::NIGHT_SHOWERS;
    case 95: return day ? WI::DAY_THUNDER : WI::NIGHT_THUNDER;
    case 96: case 99: return WI::THUNDER;
    default: return WI::NA;
  }
}

const char* compass(int deg) {
  static const char* const NAMES[] = {"N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
                                      "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};
  deg = ((deg % 360) + 360) % 360;
  return NAMES[((deg * 16 + 180) / 360) % 16];
}
