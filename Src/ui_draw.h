#ifndef UI_DRAW_H
#define UI_DRAW_H

#include "Middlewares/Adafruit/GFX/Adafruit_GFX.h"
#include "Middlewares/Adafruit/ILI9341/Adafruit_ILI9341.h"
#include "ui_config.h" // For color and dimension constants
#include "Scope.h" // For Oscilloscope status
#include "LogicAnalyzer.h" // For LogicAnalyzer status

// Initialization
void init_ui(Adafruit_ILI9341* tft_handle);

// Main drawing functions for each mode
void draw_main_menu();
void draw_oscilloscope_ui(Oscilloscope* scope); // Pass scope to get status
void draw_logic_analyzer_ui(LogicAnalyzer* la); // Pass LA to get status

// Helper function
void draw_button(int16_t x, int16_t y, int16_t w, int16_t h, const char* label, bool inverted);

#endif // UI_DRAW_H
