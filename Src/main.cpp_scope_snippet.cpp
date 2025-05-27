/* USER CODE BEGIN Includes */
#include "Middlewares/ArduinoHAL/Arduino_STM32_HAL.h"
#include "Middlewares/Adafruit/GFX/Adafruit_GFX.h"
#include "Middlewares/Adafruit/ILI9341/Adafruit_ILI9341.h"
#include "Middlewares/XPT2046/XPT2046_Touchscreen.h"
#include "Scope.h" // Include the Oscilloscope class header
/* USER CODE END Includes */

/* USER CODE BEGIN PV */
// TFT object
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS_PIN_ALIAS, TFT_DC_PIN_ALIAS, TFT_RST_PIN_ALIAS);

// Touchscreen object
XPT2046_Touchscreen ts = XPT2046_Touchscreen(
    XPT2046_CS_PIN_ALIAS, XPT2046_IRQ_PIN_ALIAS, XPT2046_CLK_PIN_ALIAS,
    XPT2046_DIN_PIN_ALIAS, XPT2046_DO_PIN_ALIAS
);

// Oscilloscope object
// Ensure hadc1 is defined (typically in main.c/adc.c by CubeMX)
// and tft is initialized.
extern ADC_HandleTypeDef hadc1; // Declare hadc1 if not defined in this file scope
Oscilloscope myScope(&hadc1, &tft);

/* USER CODE END PV */

// --- HAL ADC DMA Callbacks ---
/**
  * @brief  Conversion complete callback in non blocking mode
  * @param  hadc: ADC handle
  * @retval None
  */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
  if (hadc->Instance == ADC1) { // Check if it's the ADC used by the scope
    myScope.HAL_ADC_ConvCpltCallback_Forwarder();
  }
}

/**
  * @brief  Conversion DMA half-transfer callback in non blocking mode
  * @param  hadc: ADC handle
  * @retval None
  */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc) {
  if (hadc->Instance == ADC1) { // Check if it's the ADC used by the scope
    myScope.HAL_ADC_ConvHalfCpltCallback_Forwarder();
  }
}


int main(void) {
  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();

  /* Enable DWT for delayMicroseconds */
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  DWT->CYCCNT = 0;

  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();   // Ensure DMA is initialized BEFORE ADC if ADC uses DMA
  MX_SPI1_Init();  // For TFT
  MX_ADC1_Init();  // For Scope

  /* USER CODE BEGIN 2 */
  // Initialize TFT display
  digitalWrite(TFT_CS_PIN_ALIAS, HIGH); // Deselect TFT initially
  tft.begin();
  tft.fillScreen(ILI9341_BLACK); // Clear screen

  // Initialize Touchscreen (optional for scope, but if used elsewhere)
  // ts.begin(); 
  // ts.setCalibration(...);

  // Initialize Oscilloscope
  myScope.begin(); // Starts ADC DMA for the scope

  // Set initial trigger settings
  myScope.setTrigger(2048, Oscilloscope::RISING); // Trigger at mid-level (12-bit ADC), rising edge

  // Draw the oscilloscope grid once
  myScope.drawGrid();

  tft.setCursor(10, screen_height - 20); // Position for status text
  tft.setTextColor(ILI9341_YELLOW);
  tft.setTextSize(1);
  tft.println("Scope Running...");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    /* USER CODE END WHILE */
    /* USER CODE BEGIN 3 */
    myScope.process(); // Process oscilloscope data and update display

    // You can add other logic here, e.g., handling touch input to change scope settings
    // if (ts.touched()) {
    //   TS_Point p = ts.getPoint();
    //   // Example: if touch in certain area, change trigger level
    //   if (p.x > 200 && p.y < 50 && p.z > ts.pressureThreshold) { // Example area
    //      int new_trigger_level = myScope.getTriggerLevel() + 100;
    //      if (new_trigger_level > 4000) new_trigger_level = 100;
    //      myScope.setTrigger(new_trigger_level, myScope.getTriggerEdge());
    //      
    //      // Update display of trigger level
    //      tft.fillRect(10, screen_height - 10, 100, 10, SCOPE_BG_COLOR);
    //      tft.setCursor(10, screen_height - 10);
    //      tft.printf("Trig: %d", new_trigger_level);
    //
    //      myScope.drawGrid(); // Redraw grid to clear old waveform if settings change significantly
    //   }
    //   // Debounce or wait for touch release
    //   while(ts.touched()) { HAL_Delay(10); }
    // }

    // HAL_Delay(1); // Small delay if CPU load is an issue, but scope process should be fast
  }
  /* USER CODE END 3 */
}

// Ensure SystemClock_Config(), MX_GPIO_Init(), MX_DMA_Init(), MX_SPI1_Init(), MX_ADC1_Init()
// are correctly defined (typically by STM32CubeMX).
// Ensure `hadc1` (ADC_HandleTypeDef) is globally available or passed correctly.
// The `adc.h` should declare `extern ADC_HandleTypeDef hadc1;` if `MX_ADC1_Init()` is in `adc.c`.
// If `MX_ADC1_Init()` is in `main.c`, then `hadc1` might be file static unless made extern.
// For this snippet, `extern ADC_HandleTypeDef hadc1;` is used, assuming it's defined in `adc.c`.

/* End of main.cpp_scope_snippet.cpp */
