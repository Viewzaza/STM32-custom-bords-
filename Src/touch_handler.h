#ifndef TOUCH_HANDLER_H
#define TOUCH_HANDLER_H

#include "ui_config.h"     // For OperatingMode, current_mode, button definitions
#include "Scope.h"         // For myScope
#include "LogicAnalyzer.h" // For myLogicAnalyzer
#include "ui_draw.h"       // For draw_... functions

// Forward declarations of global objects (defined in main.cpp)
extern Oscilloscope myScope;
extern LogicAnalyzer myLogicAnalyzer;
extern Adafruit_ILI9341 tft; // For direct tft operations if needed by UI updates

void process_touch(int16_t tx, int16_t ty); // Use int16_t for coordinates

#endif // TOUCH_HANDLER_H
