#include "ui_draw.h"
#include "ui_config.h" // For constants
#include <stdio.h> // For sprintf

// Global static pointer to the TFT object
static Adafruit_ILI9341* _tft = nullptr;

void init_ui(Adafruit_ILI9341* tft_handle) {
    _tft = tft_handle;
}

void draw_button(int16_t x, int16_t y, int16_t w, int16_t h, const char* label, bool inverted) {
    if (!_tft) return;

    uint16_t bg_color = inverted ? UI_BUTTON_TEXT_COLOR : UI_BUTTON_COLOR;
    uint16_t text_color = inverted ? UI_BUTTON_COLOR : UI_BUTTON_TEXT_COLOR;

    _tft->fillRect(x, y, w, h, bg_color);
    _tft->drawRect(x, y, w, h, UI_BUTTON_TEXT_COLOR); // Border

    _tft->setTextColor(text_color);
    _tft->setTextSize(1); // Adjust size as needed
    
    // Calculate text position to center it
    int16_t text_x, text_y;
    uint16_t text_w, text_h;
    _tft->getTextBounds(label, 0, 0, &text_x, &text_y, &text_w, &text_h);
    text_x = x + (w - text_w) / 2;
    text_y = y + (h - text_h) / 2;
    
    _tft->setCursor(text_x, text_y);
    _tft->print(label);
}

void draw_main_menu() {
    if (!_tft) return;

    _tft->fillScreen(UI_BG_COLOR);
    _tft->setCursor(BTN_MENU_CENTER_X - 50, BTN_MENU_SCOPE_Y - 40); // Adjust position
    _tft->setTextColor(UI_TEXT_COLOR);
    _tft->setTextSize(2);
    _tft->println("Main Menu");

    draw_button(BTN_MENU_SCOPE_X, BTN_MENU_SCOPE_Y, BTN_MENU_SCOPE_W, BTN_MENU_SCOPE_H, "Oscilloscope", false);
    draw_button(BTN_MENU_LA_X, BTN_MENU_LA_Y, BTN_MENU_LA_W, BTN_MENU_LA_H, "Logic Analyzer", false);
}

void draw_oscilloscope_ui(Oscilloscope* scope) {
    if (!_tft || !scope) return;

    // No full screen clear here, assume scope.drawGrid() handles the waveform area.
    // Clear only the button area if necessary, or rely on buttons overwriting.
    // For simplicity, we'll redraw all UI elements.
    // A more optimized version might only update changed elements.

    // Clear bottom button bar area
    _tft->fillRect(0, SCREEN_HEIGHT_HW - BTN_HEIGHT - BTN_PADDING * 2, SCREEN_WIDTH_HW, BTN_HEIGHT + BTN_PADDING * 2, UI_BG_COLOR);
    // Clear top status bar area
    _tft->fillRect(0, 0, SCREEN_WIDTH_HW, BTN_PADDING + 10, UI_BG_COLOR);


    draw_button(BTN_SCOPE_MENU_X, BTN_SCOPE_MENU_Y, BTN_SCOPE_MENU_W, BTN_SCOPE_MENU_H, "Menu", false);
    
    const char* run_stop_label = scope->is_running() ? "Stop" : "Run";
    draw_button(BTN_SCOPE_RUNSTOP_X, BTN_SCOPE_RUNSTOP_Y, BTN_SCOPE_RUNSTOP_W, BTN_SCOPE_RUNSTOP_H, run_stop_label, false);

    const char* edge_label;
    switch (scope->getTriggerEdge()) {
        case Oscilloscope::RISING: edge_label = "Rising"; break;
        case Oscilloscope::FALLING: edge_label = "Falling"; break;
        default: edge_label = "N/A"; break;
    }
    draw_button(BTN_SCOPE_TRIGEDGE_X, BTN_SCOPE_TRIGEDGE_Y, BTN_SCOPE_TRIGEDGE_W, BTN_SCOPE_TRIGEDGE_H, edge_label, false);

    // Display Status
    _tft->setCursor(SCOPE_STATUS_X, SCOPE_STATUS_Y);
    _tft->setTextColor(UI_TEXT_COLOR);
    _tft->setTextSize(1);
    char status_buf[50];
    sprintf(status_buf, "Scope: %s | Trig Lvl: %d",
            scope->is_running() ? "Running" : "Stopped",
            scope->getTriggerLevel());
    _tft->print(status_buf);
}

void draw_logic_analyzer_ui(LogicAnalyzer* la) {
    if (!_tft || !la) return;

    // Clear bottom button bar area
    _tft->fillRect(0, SCREEN_HEIGHT_HW - BTN_HEIGHT - BTN_PADDING * 2, SCREEN_WIDTH_HW, BTN_HEIGHT + BTN_PADDING * 2, UI_BG_COLOR);
    // Clear top status bar area
    _tft->fillRect(0, 0, SCREEN_WIDTH_HW, BTN_PADDING + 10, UI_BG_COLOR);

    draw_button(BTN_LA_MENU_X, BTN_LA_MENU_Y, BTN_LA_MENU_W, BTN_LA_MENU_H, "Menu", false);
    
    const char* arm_label = la->is_capturing() ? "Capturing" : (la->is_capture_done() ? "Done" : "Arm");
    draw_button(BTN_LA_ARM_X, BTN_LA_ARM_Y, BTN_LA_ARM_W, BTN_LA_ARM_H, arm_label, la->is_capturing()); // Invert if capturing

    // Display Status
    _tft->setCursor(LA_STATUS_X, LA_STATUS_Y);
    _tft->setTextColor(UI_TEXT_COLOR);
    _tft->setTextSize(1);
    const char* status_str;
    if (la->is_capturing()) {
        status_str = "LA: Capturing...";
    } else if (la->is_capture_done()) {
        status_str = "LA: Capture Done. Pending Display.";
        if(!la->is_display_pending()){ // If display has been handled
             status_str = "LA: Capture Done. Press Arm.";
        }
    } else {
        status_str = "LA: Idle. Press Arm.";
    }
    _tft->print(status_str);
}
