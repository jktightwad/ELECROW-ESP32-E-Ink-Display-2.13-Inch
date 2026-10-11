#pragma once
#include "weather.h"

// The weather location, saved in /location.txt on the board's file system
// (label, latitude, longitude, "auto" or "fixed" on separate lines).
bool loadLocation(Location& out);
void saveLocation(const Location& loc);
