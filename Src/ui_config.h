#ifndef UI_CONFIG_H
#define UI_CONFIG_H

#include "Middlewares/Adafruit/ILI9341/Adafruit_ILI9341.h" // For color definitions

// Operating Mode Enum
enum OperatingMode {
    MODE_MENU,
    MODE_OSCILLOSCOPE,
    MODE_LOGIC_ANALYZER
};

// Global current mode variable (defined in main.cpp)
extern OperatingMode current_mode;

// UI Colors
#define UI_BG_COLOR         ILI9341_BLACK
#define UI_BUTTON_COLOR     ILI9341_DARKGREY
#define UI_BUTTON_PRESS_COLOR ILI9341_LIGHTGREY // For visual feedback on touch (not implemented yet)
#define UI_BUTTON_TEXT_COLOR ILI9341_WHITE
#define UI_TEXT_COLOR       ILI9341_WHITE
#define UI_STATUS_COLOR_OK  ILI9341_GREEN
#define UI_STATUS_COLOR_WARN ILI9341_YELLOW
#define UI_STATUS_COLOR_ERR ILI9341_RED


// Common Button Dimensions (example values, adjust as needed)
#define BTN_WIDTH        100
#define BTN_HEIGHT       40
#define BTN_PADDING      10  // Padding around buttons or between elements
#define SCREEN_WIDTH_HW  240 // Hardware screen width (e.g. ILI9341 portrait)
#define SCREEN_HEIGHT_HW 320 // Hardware screen height

// --- Main Menu Button Coordinates ---
// (Assuming a 240x320 portrait screen)
#define BTN_MENU_CENTER_X (SCREEN_WIDTH_HW / 2)

#define BTN_MENU_SCOPE_X  (BTN_MENU_CENTER_X - BTN_WIDTH / 2)
#define BTN_MENU_SCOPE_Y  (SCREEN_HEIGHT_HW / 2 - BTN_HEIGHT - BTN_PADDING / 2)
#define BTN_MENU_SCOPE_W  BTN_WIDTH
#define BTN_MENU_SCOPE_H  BTN_HEIGHT

#define BTN_MENU_LA_X     (BTN_MENU_CENTER_X - BTN_WIDTH / 2)
#define BTN_MENU_LA_Y     (SCREEN_HEIGHT_HW / 2 + BTN_PADDING / 2)
#define BTN_MENU_LA_W     BTN_WIDTH
#define BTN_MENU_LA_H     BTN_HEIGHT

// --- Oscilloscope UI Button Coordinates ---
// (Bottom row buttons)
#define SCOPE_BTN_Y       (SCREEN_HEIGHT_HW - BTN_HEIGHT - BTN_PADDING)
#define SCOPE_BTN_WIDTH   70 // Smaller buttons for more options
#define SCOPE_BTN_HEIGHT  BTN_HEIGHT

#define BTN_SCOPE_MENU_X  (BTN_PADDING)
#define BTN_SCOPE_MENU_Y  SCOPE_BTN_Y
#define BTN_SCOPE_MENU_W  SCOPE_BTN_WIDTH
#define BTN_SCOPE_MENU_H  SCOPE_BTN_HEIGHT

#define BTN_SCOPE_RUNSTOP_X (BTN_PADDING + SCOPE_BTN_WIDTH + BTN_PADDING)
#define BTN_SCOPE_RUNSTOP_Y SCOPE_BTN_Y
#define BTN_SCOPE_RUNSTOP_W SCOPE_BTN_WIDTH
#define BTN_SCOPE_RUNSTOP_H SCOPE_BTN_HEIGHT

#define BTN_SCOPE_TRIGEDGE_X (BTN_PADDING + 2 * (SCOPE_BTN_WIDTH + BTN_PADDING))
#define BTN_SCOPE_TRIGEDGE_Y SCOPE_BTN_Y
#define BTN_SCOPE_TRIGEDGE_W SCOPE_BTN_WIDTH
#define BTN_SCOPE_TRIGEDGE_H SCOPE_BTN_HEIGHT

// Status text area for Scope
#define SCOPE_STATUS_X    BTN_PADDING
#define SCOPE_STATUS_Y    BTN_PADDING // Top of screen

// --- Logic Analyzer UI Button Coordinates ---
#define LA_BTN_Y          (SCREEN_HEIGHT_HW - BTN_HEIGHT - BTN_PADDING)
#define LA_BTN_WIDTH      100

#define BTN_LA_MENU_X     (BTN_PADDING)
#define BTN_LA_MENU_Y     LA_BTN_Y
#define BTN_LA_MENU_W     LA_BTN_WIDTH
#define BTN_LA_MENU_H     BTN_HEIGHT

#define BTN_LA_ARM_X      (BTN_PADDING + LA_BTN_WIDTH + BTN_PADDING)
#define BTN_LA_ARM_Y      LA_BTN_Y
#define BTN_LA_ARM_W      LA_BTN_WIDTH
#define BTN_LA_ARM_H      BTN_HEIGHT

// Status text area for LA
#define LA_STATUS_X       BTN_PADDING
#define LA_STATUS_Y       BTN_PADDING // Top of screen (adjust if LA grid starts high)

#endif // UI_CONFIG_H
