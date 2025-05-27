#include "touch_handler.h"
#include "ui_config.h"
#include "ui_draw.h"
#include "Scope.h"
#include "LogicAnalyzer.h"

// These are expected to be defined in main.cpp
extern OperatingMode current_mode;
extern Oscilloscope myScope;
extern LogicAnalyzer myLogicAnalyzer;
extern Adafruit_ILI9341 tft; // Used by draw functions, init_ui should have set it

// Helper function to check if touch is within a button area
bool is_touch_in_rect(int16_t tx, int16_t ty, int16_t x, int16_t y, int16_t w, int16_t h) {
    return (tx >= x && tx <= (x + w) && ty >= y && ty <= (y + h));
}

void process_touch(int16_t tx, int16_t ty) {
    // Debounce: A simple way is to wait for touch release after processing one touch.
    // More advanced debouncing might be needed. For now, action on press.

    if (current_mode == MODE_MENU) {
        if (is_touch_in_rect(tx, ty, BTN_MENU_SCOPE_X, BTN_MENU_SCOPE_Y, BTN_MENU_SCOPE_W, BTN_MENU_SCOPE_H)) {
            current_mode = MODE_OSCILLOSCOPE;
            // myScope.begin(); // Start() is now separate from begin(), begin() is for one-time init
            myScope.start();    // Ensure ADC is running
            myScope.drawGrid(); // Redraw scope background (clears screen too)
            draw_oscilloscope_ui(&myScope); // Draw specific UI
        } else if (is_touch_in_rect(tx, ty, BTN_MENU_LA_X, BTN_MENU_LA_Y, BTN_MENU_LA_W, BTN_MENU_LA_H)) {
            current_mode = MODE_LOGIC_ANALYZER;
            // For LA, draw_grid is part of its display() method usually.
            // We need an initial screen setup.
            // myLogicAnalyzer.draw_grid_background(); // A new method perhaps, or ensure display() can draw empty grid.
            // For now, let draw_logic_analyzer_ui handle the initial clear & button draw.
            // The LA will be in idle state initially.
            tft.fillScreen(LA_BG_COLOR); // Clear screen for LA mode
            myLogicAnalyzer.draw_grid_static(); // A method to draw just the static grid lines and channel names
            draw_logic_analyzer_ui(&myLogicAnalyzer);
        }
    } else if (current_mode == MODE_OSCILLOSCOPE) {
        if (is_touch_in_rect(tx, ty, BTN_SCOPE_MENU_X, BTN_SCOPE_MENU_Y, BTN_SCOPE_MENU_W, BTN_SCOPE_MENU_H)) {
            myScope.stop(); // Stop ADC when leaving scope mode
            current_mode = MODE_MENU;
            draw_main_menu();
        } else if (is_touch_in_rect(tx, ty, BTN_SCOPE_RUNSTOP_X, BTN_SCOPE_RUNSTOP_Y, BTN_SCOPE_RUNSTOP_W, BTN_SCOPE_RUNSTOP_H)) {
            if (myScope.is_running()) {
                myScope.stop();
            } else {
                myScope.start();
            }
            draw_oscilloscope_ui(&myScope); // Redraw to update button label and status
        } else if (is_touch_in_rect(tx, ty, BTN_SCOPE_TRIGEDGE_X, BTN_SCOPE_TRIGEDGE_Y, BTN_SCOPE_TRIGEDGE_W, BTN_SCOPE_TRIGEDGE_H)) {
            Oscilloscope::TriggerEdge current_edge = myScope.getTriggerEdge();
            Oscilloscope::TriggerEdge next_edge = (current_edge == Oscilloscope::RISING) ? Oscilloscope::FALLING : Oscilloscope::RISING;
            myScope.setTrigger(myScope.getTriggerLevel(), next_edge); // Level remains same, edge changes
            draw_oscilloscope_ui(&myScope); // Redraw to update button label
        }
        // Add more buttons here: Trigger Level Up/Down etc.
        // Example:
        // else if (is_touch_in_rect(tx, ty, BTN_SCOPE_TRIG_LVL_UP_X, ...)) {
        //    myScope.setTrigger(myScope.getTriggerLevel() + 100, myScope.getTriggerEdge());
        //    draw_oscilloscope_ui(&myScope);
        // }

    } else if (current_mode == MODE_LOGIC_ANALYZER) {
        if (is_touch_in_rect(tx, ty, BTN_LA_MENU_X, BTN_LA_MENU_Y, BTN_LA_MENU_W, BTN_LA_MENU_H)) {
            myLogicAnalyzer.stop(); // Stop LA timer when leaving mode
            current_mode = MODE_MENU;
            draw_main_menu();
        } else if (is_touch_in_rect(tx, ty, BTN_LA_ARM_X, BTN_LA_ARM_Y, BTN_LA_ARM_W, BTN_LA_ARM_H)) {
            // Only arm if not already capturing and not pending display from a finished capture
            if (!myLogicAnalyzer.is_capturing() && !myLogicAnalyzer.is_display_pending()) {
                // Before starting a new capture, clear the old waveform area by drawing the grid
                // This assumes draw_grid_static clears the waveform portion.
                myLogicAnalyzer.draw_grid_static(); // Redraw background grid
                myLogicAnalyzer.begin(1000000); // Start 1MHz capture (or last used frequency)
            } else if (myLogicAnalyzer.is_capture_done() && myLogicAnalyzer.is_display_pending()){
                // If capture is done and pending display, this button could act as "show results"
                // or "clear display and re-arm". Current logic in main loop handles display.
                // This button press could clear the "display pending" state to allow re-arming.
                myLogicAnalyzer.acknowledge_display_done_btn_press(); // New function to allow re-arm via button
            }
            draw_logic_analyzer_ui(&myLogicAnalyzer); // Redraw to update button label and status
        }
    }
}
