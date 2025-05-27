#include "Scope.h"
#include <string.h> // For memcpy

// Constructor
Oscilloscope::Oscilloscope(ADC_HandleTypeDef* hadc_ptr, Adafruit_ILI9341* tft_display)
    : hadc(hadc_ptr),
      tft(tft_display),
      dma_cplt_flag(false),
      dma_half_cplt_flag(false),
      trigger_level(2048), // Default trigger level (mid-point for 12-bit ADC)
      trigger_edge(RISING),
      triggered_once(false),
      last_trigger_time(0),
      search_offset_first_half(0), 
      search_offset_second_half(0),
      is_running_flag(false) { // Initialize is_running_flag to false
    // Initialize screen dimensions from TFT object
    if (tft) {
        screen_width = tft->width();
        screen_height = tft->height();
    } else {
        screen_width = 320; // Default if tft is null for some reason
        screen_height = 240;
    }

    // Define waveform display area (e.g., full screen or a sub-rectangle)
    // For this example, let's use a small margin.
    wave_x = 5;
    wave_y = 5;
    wave_w = screen_width - 10;
    wave_h = screen_height - 10;
    
    // Ensure display_buffer in Scope.h is large enough for screen_width.
    // The current display_buffer[320] in Scope.h should be fine for typical screens like 240x320 or 320x240.
    // If screen_width > 320, this buffer would be too small.
    // A more robust solution would dynamically allocate or use a std::vector if heap is available.
    // For now, we rely on display_buffer being large enough.
}

// Initialization
void Oscilloscope::begin() {
    if (!hadc) return; // Safety check

    // Start ADC with DMA
    HAL_StatusTypeDef status = HAL_ADC_Start_DMA(hadc, (uint32_t*)adc_buffer, ADC_BUFFER_SIZE);
    if (status != HAL_OK) {
        // Handle ADC start error if necessary
        // For example, print to UART or display an error on TFT
        if (tft) {
            tft->setCursor(10, 10);
            tft->setTextColor(ILI9341_RED);
            tft->setTextSize(1);
            tft->printf("ADC DMA Start Error: %d", status);
        }
        return;
    }
    // Initialize search offsets to prevent immediate trigger on stale buffer data if ADC starts mid-way
    // Or, more simply, always start searching from the beginning of a fresh buffer half.
    // The current initialization to 0 is fine if ADC starts cleanly.
    // If ADC might have old data on startup, could initialize to ADC_BUFFER_SIZE / 2 -1 to skip most of it initially.
    // For this implementation, starting at 0 for each new half-buffer is the design.
}

// Control methods
void Oscilloscope::start() {
    if (!hadc || is_running_flag) return;
    HAL_StatusTypeDef status = HAL_ADC_Start_DMA(hadc, (uint32_t*)adc_buffer, ADC_BUFFER_SIZE);
    if (status == HAL_OK) {
        is_running_flag = true;
        // Reset DMA flags and search offsets upon starting
        dma_cplt_flag = false;
        dma_half_cplt_flag = false;
        search_offset_first_half = 0;
        search_offset_second_half = 0;
    } else {
        // Handle ADC start error
        if (tft) {
            // This is just an example, real error handling might be more sophisticated
            tft->setCursor(10, screen_height - 10); // Assuming screen_height is initialized
            tft->setTextColor(ILI9341_RED);
            tft->setTextSize(1);
            tft->printf("ADC Start Err: %d", status);
        }
    }
}

void Oscilloscope::stop() {
    if (!hadc || !is_running_flag) return;
    HAL_StatusTypeDef status = HAL_ADC_Stop_DMA(hadc);
    if (status == HAL_OK) {
        is_running_flag = false;
    } else {
        // Handle ADC stop error
         if (tft) {
            tft->setCursor(10, screen_height - 10);
            tft->setTextColor(ILI9341_RED);
            tft->setTextSize(1);
            tft->printf("ADC Stop Err: %d", status);
        }
    }
}


// Initialization - now primarily sets up and can optionally start
void Oscilloscope::begin() {
    // Initial one-time setup if any is needed beyond constructor.
    // For now, start() handles the ADC DMA start.
    // If begin() is meant to be the primary way to kick things off:
    // this->start(); 
    // However, UI logic might call start/stop separately.
    // Let's make begin() ensure it's initially stopped or in a defined state.
    is_running_flag = false; // Ensure it starts in a known state
    // The actual ADC start will be triggered by user via UI -> myScope.start()
}


// DMA Callback Forwarders
void Oscilloscope::HAL_ADC_ConvCpltCallback_Forwarder() {
    dma_cplt_flag = true;
    search_offset_second_half = 0; // Reset search offset for the second half, as it's newly filled
}

void Oscilloscope::HAL_ADC_ConvHalfCpltCallback_Forwarder() {
    dma_half_cplt_flag = true;
    search_offset_first_half = 0; // Reset search offset for the first half, as it's newly filled
}

// Set Trigger
void Oscilloscope::setTrigger(int level, TriggerEdge edge) {
    if (level < 0) level = 0;
    if (level > 4095) level = 4095; // STM32 12-bit ADC
    trigger_level = level;
    trigger_edge = edge;
}

// Find Trigger
int Oscilloscope::findTrigger(uint16_t* buffer_to_search, int buffer_len, int search_offset) {
    // buffer_len is typically ADC_BUFFER_SIZE / 2
    // search_offset is where to start searching in this half-buffer
    for (int i = search_offset; i < buffer_len - 1; ++i) { // -1 because we check buffer[i+1]
        uint16_t prev_sample = buffer_to_search[i];
        uint16_t current_sample = buffer_to_search[i + 1];

        if (trigger_edge == RISING) {
            if (prev_sample < trigger_level && current_sample >= trigger_level) {
                return i + 1; // Return index of the sample that crossed the threshold
            }
        } else if (trigger_edge == FALLING) {
            if (prev_sample > trigger_level && current_sample <= trigger_level) {
                return i + 1; // Return index of the sample that crossed the threshold
            }
        }
    }
    return -1; // Trigger not found
}


// Prepare display data (scaling and copying)
void Oscilloscope::prepareDisplayData(uint16_t* src_buffer_half, int src_buffer_half_len, int trigger_idx_in_half) {
    // src_buffer_half points to the start of the half (first or second) of adc_buffer
    // src_buffer_half_len is ADC_BUFFER_SIZE / 2
    // trigger_idx_in_half is the trigger index within that specific half (0 to src_buffer_half_len -1)

    // Clear display_buffer (optional, if not all points are filled)
    // memset(display_buffer, 0, sizeof(display_buffer)); // Or fill with a value indicating no data

    int display_width = wave_w; // Use waveform area width for display points
    if (display_width > 320) display_width = 320; // Cap at physical buffer size from Scope.h

    // Calculate how many samples to show before the trigger point
    // For simplicity, let's try to center the trigger point if possible, or place it at 1/4th of the screen.
    int pre_trigger_samples = display_width / 4;

    for (int i = 0; i < display_width; ++i) {
        // Calculate the effective index in the full adc_buffer
        // This needs to handle wrapping around the adc_buffer correctly.
        // Let full_trigger_idx be the trigger index in the full adc_buffer (0 to ADC_BUFFER_SIZE-1)
        // If processing first half, full_trigger_idx = trigger_idx_in_half
        // If processing second half, full_trigger_idx = (ADC_BUFFER_SIZE/2) + trigger_idx_in_half

        // This calculation determines which sample from adc_buffer maps to display_buffer[i]
        int adc_buffer_sample_idx;
        
        // If src_buffer_half points to the beginning of adc_buffer (first half processing)
        if (src_buffer_half == adc_buffer) {
            adc_buffer_sample_idx = trigger_idx_in_half - pre_trigger_samples + i;
        } else { // src_buffer_half points to the middle of adc_buffer (second half processing)
            adc_buffer_sample_idx = (ADC_BUFFER_SIZE / 2) + trigger_idx_in_half - pre_trigger_samples + i;
        }

        // Handle wrap-around for the adc_buffer (circular buffer logic)
        adc_buffer_sample_idx = (adc_buffer_sample_idx + ADC_BUFFER_SIZE) % ADC_BUFFER_SIZE;
        
        uint16_t adc_sample = adc_buffer[adc_buffer_sample_idx];

        // Scale ADC sample to screen coordinates (waveform height)
        // Invert Y-axis: 0 ADC -> top of wave_h, 4095 ADC -> bottom of wave_h
        display_buffer[i] = (uint16_t)(wave_h - (( (float)adc_sample * wave_h) / 4095.0f));
        if (display_buffer[i] >= wave_h) display_buffer[i] = wave_h -1; // clamp
        if (display_buffer[i] < 0) display_buffer[i] = 0; // clamp
    }
}


// Draw Grid
void Oscilloscope::drawGrid() {
    if (!tft) return;

    tft->fillRect(wave_x, wave_y, wave_w, wave_h, SCOPE_BG_COLOR);

    // Draw grid lines
    int num_horizontal_lines = 5; // Example
    int num_vertical_lines = 10;  // Example

    // Horizontal lines
    for (int i = 0; i <= num_horizontal_lines; ++i) {
        int16_t y_pos = wave_y + (i * wave_h / num_horizontal_lines);
        if (i == num_horizontal_lines) y_pos -=1; // Ensure last line is visible
        tft->drawHorizontalLine(wave_x, y_pos, wave_w, SCOPE_GRID_COLOR);
    }

    // Vertical lines
    for (int i = 0; i <= num_vertical_lines; ++i) {
        int16_t x_pos = wave_x + (i * wave_w / num_vertical_lines);
        if (i == num_vertical_lines) x_pos -=1; // Ensure last line is visible
        tft->drawVerticalLine(x_pos, wave_y, wave_h, SCOPE_GRID_COLOR);
    }
    triggered_once = false; // Reset this so waveform clears correctly first time after grid
}

// Draw Waveform
void Oscilloscope::drawWaveform(uint16_t* data, int data_len, uint16_t color) {
    if (!tft) return;

    // Clear only the waveform area (already done by drawGrid if grid is redrawn,
    // but good to have if grid is static)
    // More efficiently, only clear the part of the waveform that was drawn last time.
    // For single-shot style, clearing the whole area before drawing is fine.
    if (triggered_once) { // Avoid clearing grid on first draw
         tft->fillRect(wave_x, wave_y, wave_w, wave_h, SCOPE_BG_COLOR);
         // Optional: Redraw grid if clearing the whole area.
         // For now, assume grid is static or redrawn by user explicitly.
         // If we cleared, we should redraw grid. A better way is to save background.
         // Let's redraw grid for now, or make drawGrid clear and draw.
         // The current drawGrid clears and draws. So call it before drawWaveform if clearing is needed.
         // This is inefficient. A better approach would be to draw the old waveform in BG_COLOR.
         // For now, the process() will call drawGrid() then drawWaveform().
    }


    // data_len should be wave_w
    // The `data` array contains scaled Y coordinates (0 to wave_h-1)
    for (int i = 0; i < data_len - 1; ++i) {
        // Ensure points are within the waveform display area boundaries
        int16_t y1 = wave_y + data[i];
        int16_t y2 = wave_y + data[i+1];

        // Clamp to prevent drawing outside wave_y to wave_y + wave_h
        y1 = (y1 < wave_y) ? wave_y : y1;
        y1 = (y1 >= wave_y + wave_h) ? (wave_y + wave_h - 1) : y1;
        y2 = (y2 < wave_y) ? wave_y : y2;
        y2 = (y2 >= wave_y + wave_h) ? (wave_y + wave_h - 1) : y2;
        
        tft->drawLine(wave_x + i, y1, wave_x + i + 1, y2, color);
    }
    triggered_once = true;
}

// Main processing function
void Oscilloscope::process() {
    if (!is_running_flag) {
        return; // Don't process if not running
    }

    uint16_t* buffer_to_process = nullptr;
    int buffer_half_len = ADC_BUFFER_SIZE / 2;
    int* p_current_search_offset = nullptr;

    if (dma_half_cplt_flag) {
        buffer_to_process = adc_buffer; // Process first half
        p_current_search_offset = &search_offset_first_half;
        dma_half_cplt_flag = false; // Reset flag
    } else if (dma_cplt_flag) {
        buffer_to_process = &adc_buffer[buffer_half_len]; // Process second half
        p_current_search_offset = &search_offset_second_half;
        dma_cplt_flag = false; // Reset flag
    }

    if (buffer_to_process && p_current_search_offset) {
        int trigger_idx = findTrigger(buffer_to_process, buffer_half_len, *p_current_search_offset);

        if (trigger_idx != -1) {
            // Trigger found
            prepareDisplayData(buffer_to_process, buffer_half_len, trigger_idx);
            
            drawGrid(); 
            drawWaveform(display_buffer, wave_w, SCOPE_WAVEFORM_COLOR);
            
            // Update the search offset for the half that was just processed,
            // so the next search in this same half (if re-processed before next DMA event for this half)
            // starts after the found trigger.
            *p_current_search_offset = trigger_idx + 1;
            if (*p_current_search_offset >= buffer_half_len) {
                *p_current_search_offset = buffer_half_len -1; // Start at end if goes over (no re-trigger in this half)
                                                              // Or set to 0 if a full new sweep is desired next time.
                                                              // For single shot per DMA half, this prevents re-trigger.
            }

        } else {
            // Trigger not found in this half from the current search_offset onwards.
            // Reset the search offset for this half so next time it's processed, it starts from the beginning.
            *p_current_search_offset = 0;
            
            // Optional: Implement "auto" trigger mode here.
            // If no trigger for some time, display data anyway.
            // For now, if no trigger, display remains static from last trigger.
        }
    }
}
