#ifndef SCOPE_H
#define SCOPE_H

#include "stm32f1xx_hal.h" // For ADC_HandleTypeDef
#include "Middlewares/Adafruit/GFX/Adafruit_GFX.h" // For Adafruit_ILI9341
#include "Middlewares/Adafruit/ILI9341/Adafruit_ILI9341.h" // For Adafruit_ILI9341

// Configuration constants
#define ADC_BUFFER_SIZE 1024 // Size of the DMA buffer (can be tuned)
// DISPLAY_WIDTH and DISPLAY_HEIGHT will be taken from tft object.

// Colors for the scope display
#define SCOPE_BG_COLOR       ILI9341_BLACK
#define SCOPE_GRID_COLOR     ILI9341_DARKGREY
#define SCOPE_WAVEFORM_COLOR ILI9341_GREEN

class Oscilloscope {
public:
    enum TriggerEdge {
        RISING,
        FALLING,
        NONE // For free-running mode (not implemented yet)
    };

    // Constructor
    Oscilloscope(ADC_HandleTypeDef* hadc_ptr, Adafruit_ILI9341* tft_display);

    // Initialization
    void begin(); // Starts ADC DMA, sets up initial state

    // Main processing function (call in loop)
    void process();

    // Configuration
    void setTrigger(int level, TriggerEdge edge);
    TriggerEdge getTriggerEdge() const { return trigger_edge; }
    int getTriggerLevel() const { return trigger_level; }

    // Control methods for starting/stopping ADC capture
    void start(); 
    void stop();  
    bool is_running() const { return is_running_flag; }

    // DMA Callback Forwarders - to be called by global HAL ADC Callbacks
    void HAL_ADC_ConvCpltCallback_Forwarder();
    void HAL_ADC_ConvHalfCpltCallback_Forwarder();

    // Drawing functions
    void drawGrid();
    void drawWaveform(uint16_t* display_data, int data_len, uint16_t color);

private:
    // Member variables
    ADC_HandleTypeDef* hadc;      // Pointer to the HAL ADC handle
    Adafruit_ILI9341* tft;        // Pointer to the TFT display object

    uint16_t adc_buffer[ADC_BUFFER_SIZE]; // DMA buffer for ADC samples
    volatile bool dma_cplt_flag;          // DMA transfer complete flag
    volatile bool dma_half_cplt_flag;     // DMA transfer half-complete flag

    // Trigger settings
    int trigger_level;            // ADC value (0-4095)
    TriggerEdge trigger_edge;
    bool is_running_flag;         // To control ADC capture

    // Display properties (initialized from tft object)
    int16_t screen_width;
    int16_t screen_height;

    // Waveform display area (can be a sub-rectangle of the screen)
    // For simplicity, let's assume waveform uses most of the screen for now.
    // These can be made configurable later.
    int16_t wave_x;
    int16_t wave_y;
    int16_t wave_w;
    int16_t wave_h;
    
    uint16_t display_buffer[320]; // Max common screen width. Will use up to screen_width.

    // Helper methods
    int findTrigger(uint16_t* buffer_to_search, int buffer_len, int search_offset);
    void prepareDisplayData(uint16_t* src_buffer, int src_buffer_len, int trigger_index);

    // Internal state
    bool triggered_once; // To draw grid only once initially if needed, or manage first trigger
    uint32_t last_trigger_time; // For timeout or re-arm logic (advanced)
    int search_offset_first_half; // To optimize findTrigger search in the first half
    int search_offset_second_half; // To optimize findTrigger search in the second half
};

#endif // SCOPE_H
