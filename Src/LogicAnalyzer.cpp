#include "LogicAnalyzer.h"
#include <stdio.h> // For sprintf if used for labels

// Constructor
LogicAnalyzer::LogicAnalyzer(TIM_HandleTypeDef* timer_handle, Adafruit_ILI9341* display_handle)
    : htim_sample(timer_handle),
      tft(display_handle),
      current_sample_index(0),
      current_la_status(LA_IDLE) { // Initialize status to IDLE
    if (tft) {
        screen_width = tft->width();
        screen_height = tft->height();
    } else {
        // Default values if tft is not available (should not happen in normal operation)
        screen_width = 320;
        screen_height = 240;
    }
    channel_height = screen_height / LA_NUM_CHANNELS;
    wave_area_x_start = 30; // Small margin for channel names/labels
    wave_area_width = screen_width - wave_area_x_start - 5; // And a bit of end margin

    // Ensure LA_BUFFER_SAMPLES is not greater than drawable width
    if (LA_BUFFER_SAMPLES > wave_area_width) {
        // This would be an issue, ideally LA_BUFFER_SAMPLES should match screen width pixels available for waveform
        // For now, we proceed, but drawing might be truncated or require scaling.
        // The current design draws 1 sample per horizontal pixel up to LA_BUFFER_SAMPLES or wave_area_width.
    }
}

// Control methods
void LogicAnalyzer::begin(uint32_t sample_freq_hz) {
    if (!htim_sample || current_la_status == LA_CAPTURING) return; // Don't restart if already capturing

    current_sample_index = 0;
    current_la_status = LA_CAPTURING;

    // Configure timer PSC and ARR
    // Assuming PCLK1 is the clock source for TIM2 (common case)
    // And PCLK1 is 72MHz as discussed.
    uint32_t timer_clock_freq = HAL_RCC_GetPCLK1Freq(); 
    // If timer is TIM1 (APB2), use HAL_RCC_GetPCLK2Freq()
    // For STM32F103, if APB1 Prescaler is /1, PCLK1 = HCLK (e.g. 72MHz)
    // If APB1 Prescaler is /2, PCLK1 = HCLK/2 (e.g. 36MHz)
    // The previous .ioc file set APB1 Prescaler to /2 (36MHz), and APB2 to /1 (72MHz)
    // Let's assume TIM2 is used, so PCLK1 is 36MHz.
    // If the problem statement's 72MHz for timer is firm, PCLK1 must be 72MHz OR TIM1/TIM8 used.
    // Re-evaluating based on previous .ioc: PCLK1 = 36MHz.
    // So, for 1MHz sampling (1us period):
    // 36MHz / 1MHz = 36. So, (PSC+1)*(ARR+1) = 36.
    // If PSC = 0, ARR = 35.
    // If PSC = 1, ARR = 17. ( (1+1)*(17+1) = 2*18 = 36 )
    // Let's choose PSC=0 for higher resolution adjustments.
    // ARR = (timer_clock_freq / sample_freq_hz) - 1;

    if (timer_clock_freq == 0) { // Safety check if clock config is not found
        // Fallback or error
        return;
    }
    
    uint32_t prescaler_val = 0; // Default for simplicity
    uint32_t arr_val = (timer_clock_freq / (prescaler_val + 1) / sample_freq_hz) - 1;

    __HAL_TIM_SET_PRESCALER(htim_sample, prescaler_val);
    __HAL_TIM_SET_AUTORELOAD(htim_sample, arr_val);

    HAL_TIM_Base_Start_IT(htim_sample);
}

void LogicAnalyzer::stop() {
    if (!htim_sample) return;
    HAL_TIM_Base_Stop_IT(htim_sample);
    if (current_la_status == LA_CAPTURING) { // If stopped during capture, move to IDLE
        current_la_status = LA_IDLE;
    }
    // If stopped after capture done, status remains LA_DONE_PENDING_DISPLAY or LA_DONE_DISPLAYED
}

// Called by timer ISR
void LogicAnalyzer::process_capture_ISR() {
    if (current_la_status != LA_CAPTURING) return;

    if (current_sample_index < LA_BUFFER_SAMPLES) {
        // Read PC0-PC3 (Channels 0-3)
        uint8_t ch0_state = HAL_GPIO_ReadPin(LA_CH0_PORT, LA_CH0_PIN) == GPIO_PIN_SET ? 1 : 0;
        uint8_t ch1_state = HAL_GPIO_ReadPin(LA_CH1_PORT, LA_CH1_PIN) == GPIO_PIN_SET ? 1 : 0;
        uint8_t ch2_state = HAL_GPIO_ReadPin(LA_CH2_PORT, LA_CH2_PIN) == GPIO_PIN_SET ? 1 : 0;
        uint8_t ch3_state = HAL_GPIO_ReadPin(LA_CH3_PORT, LA_CH3_PIN) == GPIO_PIN_SET ? 1 : 0;

        la_buffer[0][current_sample_index] = ch0_state;
        la_buffer[1][current_sample_index] = ch1_state;
        la_buffer[2][current_sample_index] = ch2_state;
        la_buffer[3][current_sample_index] = ch3_state;

        current_sample_index++;
    } else { // Buffer full
        HAL_TIM_Base_Stop_IT(htim_sample); // Stop timer directly from ISR for speed
        current_la_status = LA_DONE_PENDING_DISPLAY;
    }
}

// Drawing methods
void LogicAnalyzer::draw_grid_static() {
    if (!tft) return;
    // This function should only draw static elements, not clear the whole screen unless necessary for this mode.
    // If called when switching modes, main.cpp could do tft->fillScreen(LA_BG_COLOR);
    // For now, let it clear its designated area.
    // tft->fillScreen(LA_BG_COLOR); // Or clear only waveform area if buttons are separate

    // Draw channel dividers and names
    const char* ch_names[] = {"CH0", "CH1", "CH2", "CH3"};
    uint16_t ch_colors[] = {LA_CHANNEL_COLOR_0, LA_CHANNEL_COLOR_1, LA_CHANNEL_COLOR_2, LA_CHANNEL_COLOR_3};

    for (int i = 0; i < LA_NUM_CHANNELS; ++i) {
        int16_t y_channel_mid = (i * channel_height) + (channel_height / 2);
        
        // Draw horizontal line for channel separation (optional, if channel_height is large enough)
        if (i > 0) {
            tft->drawHorizontalLine(0, i * channel_height, screen_width, LA_GRID_COLOR);
        }
        
        tft->setCursor(2, y_channel_mid - 4); // Adjust for text size
        tft->setTextColor(ch_colors[i]);
        tft->setTextSize(1);
        tft->print(ch_names[i]);
    }
    
    // Vertical grid lines for time (optional)
    // int num_vertical_lines = 10; // Example for time grid
    // for (int i = 0; i <= num_vertical_lines; ++i) {
    //     int16_t x_pos = wave_area_x_start + (i * wave_area_width / num_vertical_lines);
    //     if (i == num_vertical_lines) x_pos = wave_area_x_start + wave_area_width -1;
    //     tft->drawVerticalLine(x_pos, 0, screen_height, LA_GRID_COLOR); // Full height for LA
    // }
}

void LogicAnalyzer::draw_waveforms() {
    if (!tft) return;
    
    // Clear only the waveform area before drawing new waveforms
    // Assuming channel names and static grid are outside this specific waveform area if not redrawing them.
    // For simplicity, if draw_grid_static was called, it might have prepared the background.
    // If this is called repeatedly for "live" view (not current design), this clear is essential.
    _tft->fillRect(wave_area_x_start, 0, wave_area_width, screen_height, LA_BG_COLOR);


    uint16_t ch_colors[] = {LA_CHANNEL_COLOR_0, LA_CHANNEL_COLOR_1, LA_CHANNEL_COLOR_2, LA_CHANNEL_COLOR_3};
    int16_t y_offset_high = channel_height / 4;      // Position for logic HIGH line within a channel's slot
    int16_t y_offset_low = (channel_height * 3) / 4; // Position for logic LOW line

    int samples_to_draw = (current_sample_index < wave_area_width) ? current_sample_index : wave_area_width;
    if (samples_to_draw > LA_BUFFER_SAMPLES) samples_to_draw = LA_BUFFER_SAMPLES;


    for (int ch = 0; ch < LA_NUM_CHANNELS; ++ch) {
        int16_t y_channel_base = ch * channel_height;
        int16_t prev_y = 0;

        for (int i = 0; i < samples_to_draw; ++i) {
            int16_t current_y_pos;
            if (la_buffer[ch][i] == 1) { // Logic HIGH
                current_y_pos = y_channel_base + y_offset_high;
            } else { // Logic LOW
                current_y_pos = y_channel_base + y_offset_low;
            }

            if (i > 0) { // Draw line from previous sample to current
                // Vertical line for transition
                if (prev_y != current_y_pos) {
                    tft->drawVerticalLine(wave_area_x_start + i, 
                                          (prev_y < current_y_pos ? prev_y : current_y_pos), 
                                          abs(current_y_pos - prev_y), 
                                          ch_colors[ch]);
                }
                // Horizontal line for current state
                tft->drawHorizontalLine(wave_area_x_start + i, current_y_pos, 1, ch_colors[ch]);
            } else { // First sample
                 tft->drawHorizontalLine(wave_area_x_start + i, current_y_pos, 1, ch_colors[ch]);
            }
            prev_y = current_y_pos;
        }
    }
}


void LogicAnalyzer::display() {
    if (!tft || !la_capture_done_flag) return;

    draw_grid();      // Clear screen and draw grid/labels
    draw_waveforms(); // Draw the captured waveforms

    // After displaying, the data is considered viewed.
    // current_la_status remains LA_DONE_PENDING_DISPLAY until acknowledge_display_done() is called
}


// Status and control methods
LogicAnalyzer::LA_Status LogicAnalyzer::get_status() const {
    return current_la_status;
}

void LogicAnalyzer::acknowledge_display_done() {
    if (current_la_status == LA_DONE_PENDING_DISPLAY) {
        current_la_status = LA_DONE_DISPLAYED;
    }
}

void LogicAnalyzer::arm_new_capture() {
    // This function is called when user wants to start a new capture,
    // typically after results have been shown (LA_DONE_DISPLAYED) or if idle.
    if (current_la_status == LA_IDLE || current_la_status == LA_DONE_DISPLAYED) {
        // Optionally, clear screen or specific areas before starting new capture
        // draw_grid_static(); // Prepare background
        // tft->fillRect(wave_area_x_start, 0, wave_area_width, screen_height, LA_BG_COLOR); // Clear old waves
        
        // The begin method will set status to LA_CAPTURING
        // begin(1000000); // Default to 1MHz or use a stored/configured frequency
    }
    // If currently LA_DONE_PENDING_DISPLAY, the user should see results first or explicitly cancel.
    // UI logic in touch_handler will manage this. If "Arm" is pressed while pending display,
    // it might mean "discard current results and re-arm".
    // For now, arm_new_capture is simple; begin() is the main entry to start capture.
}

// Deprecated old helper methods (replaced by get_status())
// bool LogicAnalyzer::is_capturing() const {
//     return current_la_status == LA_CAPTURING;
// }
// bool LogicAnalyzer::is_capture_done() const {
//     return current_la_status == LA_DONE_PENDING_DISPLAY || current_la_status == LA_DONE_DISPLAYED;
// }
// bool LogicAnalyzer::is_display_pending() const {
//    return current_la_status == LA_DONE_PENDING_DISPLAY;
// }
