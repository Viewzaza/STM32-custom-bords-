#ifndef LOGIC_ANALYZER_H
#define LOGIC_ANALYZER_H

#include "stm32f1xx_hal.h" // For TIM_HandleTypeDef, GPIO
#include "Middlewares/Adafruit/GFX/Adafruit_GFX.h"
#include "Middlewares/Adafruit/ILI9341/Adafruit_ILI9341.h"

// Configuration constants
#define LA_NUM_CHANNELS 4
#define LA_BUFFER_SAMPLES 320 // Max samples, typically screen width

// GPIO Pin definitions for Logic Analyzer Channels (PC0-PC3)
// These are logical definitions; the actual CubeMX init sets them as inputs.
#define LA_CH0_PORT GPIOC
#define LA_CH0_PIN  GPIO_PIN_0
#define LA_CH1_PORT GPIOC
#define LA_CH1_PIN  GPIO_PIN_1
#define LA_CH2_PORT GPIOC
#define LA_CH2_PIN  GPIO_PIN_2
#define LA_CH3_PORT GPIOC
#define LA_CH3_PIN  GPIO_PIN_3

// Colors for Logic Analyzer display
#define LA_BG_COLOR         ILI9341_BLACK
#define LA_GRID_COLOR       ILI9341_DARKGREY
#define LA_CHANNEL_COLOR_0  ILI9341_GREEN
#define LA_CHANNEL_COLOR_1  ILI9341_YELLOW
#define LA_CHANNEL_COLOR_2  ILI9341_CYAN
#define LA_CHANNEL_COLOR_3  ILI9341_MAGENTA
#define LA_TEXT_COLOR       ILI9341_WHITE


class LogicAnalyzer {
public:
    // Constructor
    LogicAnalyzer(TIM_HandleTypeDef* timer_handle, Adafruit_ILI9341* display_handle);

    // Control methods
    void begin(uint32_t sample_freq_hz); // Starts capture
    void stop();                         // Stops capture

    // Called by timer ISR to capture one sample set across channels
    void process_capture_ISR();

    // Called from main loop when capture is done to show data
    void display();
    void draw_grid_static(); // Draws only the static parts of the grid (lines, names)

    // Status enum and helper methods
    enum LA_Status { LA_IDLE, LA_CAPTURING, LA_DONE_PENDING_DISPLAY, LA_DONE_DISPLAYED };
    LA_Status get_status() const;
    
    // bool is_capturing() const; // Replaced by get_status()
    // bool is_capture_done() const { return la_capture_done_flag; } // Replaced
    void acknowledge_display_done(); // Call after display() has been handled by main loop
    // bool is_display_pending() const { return la_display_pending; } // Replaced

    // New method for button interaction to clear "Done" state for re-arming
    void arm_new_capture();


private:
    // Member variables
    TIM_HandleTypeDef* htim_sample;      // Pointer to the HAL Timer handle
    Adafruit_ILI9341* tft;               // Pointer to the TFT display object

    uint8_t la_buffer[LA_NUM_CHANNELS][LA_BUFFER_SAMPLES]; // Buffer for captured samples
    // volatile bool la_capture_done_flag;  // Replaced by LA_Status
    // volatile bool la_display_pending;    // Replaced by LA_Status
    volatile uint32_t current_sample_index; // Current position in the buffer
    // volatile bool capturing_active;      // Replaced by LA_Status
    volatile LA_Status current_la_status; // Current operational status

    // Display properties
    int16_t screen_width;
    int16_t screen_height;
    int16_t channel_height;          // Vertical space per channel on display
    int16_t wave_area_x_start;
    int16_t wave_area_width;

    // Internal drawing methods
    void draw_grid();
    void draw_waveforms();
};

#endif // LOGIC_ANALYZER_H
