/* USER CODE BEGIN Includes */
#include "Middlewares/ArduinoHAL/Arduino_STM32_HAL.h"
#include "Middlewares/Adafruit/GFX/Adafruit_GFX.h"
#include "Middlewares/Adafruit/ILI9341/Adafruit_ILI9341.h"
// #include "Middlewares/XPT2046/XPT2046_Touchscreen.h" // If used
#include "Scope.h"          // If scope is also used
#include "LogicAnalyzer.h"  // Include the Logic Analyzer class header
/* USER CODE END Includes */

/* USER CODE BEGIN PV */
// TFT object (ensure this is the same instance used by other modules if any)
extern Adafruit_ILI9341 tft; // Assuming tft is defined and initialized elsewhere (e.g. main or scope snippet)

// Timer handle for Logic Analyzer (e.g., TIM2)
// This should be configured by STM32CubeMX.
extern TIM_HandleTypeDef htim2; // Declare if not defined in this file scope

// Logic Analyzer object
LogicAnalyzer myLogicAnalyzer(&htim2, &tft);

// Application state variables
enum AppMode { MODE_SCOPE, MODE_LOGIC_ANALYZER, MODE_IDLE };
AppMode current_mode = MODE_LOGIC_ANALYZER; // Example: Default to Logic Analyzer mode
bool la_mode_first_run = true; // Flag to handle initial setup for LA mode

/* USER CODE END PV */

// --- HAL Timer Period Elapsed Callback ---
/**
  * @brief  Period elapsed callback in non blocking mode
  * @param  htim: TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  // Check if it's the timer instance used by the Logic Analyzer (e.g., TIM2)
  if (htim->Instance == htim2.Instance) { // Ensure you use the correct timer instance
    myLogicAnalyzer.process_capture_ISR();
  }
  
  // Add other timer callbacks if needed (e.g., for other peripherals or base tick)
  // if (htim->Instance == TIM1) { // Example for another timer
  // }
}


int main(void) {
  /* MCU Configuration & Peripheral Init */
  HAL_Init();
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // For DWT cycle counter if used by other modules
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  DWT->CYCCNT = 0;
  SystemClock_Config();

  MX_GPIO_Init(); // Initializes all GPIOs including PC0-PC3 as inputs
  MX_DMA_Init();
  MX_SPI1_Init(); // For TFT
  MX_ADC1_Init(); // For Scope (if used)
  MX_TIM2_Init(); // For Logic Analyzer (ensure TIM2 is configured in CubeMX for periodic interrupts)

  /* USER CODE BEGIN 2 */
  // Initialize TFT (assuming it's not done by another module like scope already)
  // if (current_mode == MODE_LOGIC_ANALYZER) { // Or common init
  //    digitalWrite(TFT_CS_PIN_ALIAS, HIGH);
  //    tft.begin();
  //    tft.fillScreen(LA_BG_COLOR); // Initial screen clear for LA
  // }

  // Main application state machine or logic
  // This example focuses on Logic Analyzer mode operation.
  // Touch input or buttons could be used to switch modes (current_mode variable).

  /* USER CODE END 2 */

  /* Infinite loop */
  while (1) {
    /* USER CODE END WHILE */
    /* USER CODE BEGIN 3 */

    if (current_mode == MODE_LOGIC_ANALYZER) {
      if (la_mode_first_run) {
        tft.fillScreen(LA_BG_COLOR); // Clear screen for LA mode
        tft.setCursor(10, 10);
        tft.setTextColor(LA_TEXT_COLOR);
        tft.setTextSize(1);
        tft.println("Logic Analyzer Mode. Starting capture...");
        HAL_Delay(500); // Show message briefly
        
        myLogicAnalyzer.begin(1000000); // Start 1MHz capture (adjust freq as needed)
        la_mode_first_run = false;
      }

      if (myLogicAnalyzer.is_capture_done() && myLogicAnalyzer.is_display_pending()) {
        myLogicAnalyzer.display(); // Display captured data
        myLogicAnalyzer.acknowledge_display_done(); // Mark display as handled

        // After display, you might want to:
        // 1. Stop and wait for user input to start new capture.
        // 2. Automatically restart capture.
        // For this example, let's allow a new capture to be started (e.g., by button press or another trigger)
        // For simplicity here, we'll just make it ready for another 'begin' call if some condition is met.
        // To re-arm automatically:
        // HAL_Delay(1000); // Show display for a second
        // myLogicAnalyzer.begin(1000000); // Restart capture
        // Or, set a flag to allow a button press to call begin().
        tft.setCursor(screen_width - 80, screen_height - 10);
        tft.setTextColor(LA_TEXT_COLOR);
        tft.print("Capture Done!");

        // To re-trigger, you'd call myLogicAnalyzer.begin(...) again.
        // This could be tied to a button press.
        // For now, it will sit in this state until mode changes or MCU is reset.
        // Or for continuous auto-retrigger:
        // HAL_Delay(2000); // Display for 2s
        // la_mode_first_run = true; // This will trigger a new .begin() next loop iteration
        
      } else if (myLogicAnalyzer.is_capturing()) {
        // Optionally display "Capturing..." or update a live counter
        // This part is tricky with current display logic as display() redraws everything.
        // For live view, ISR would need to update a small portion of screen or use double buffering.
        // For now, we wait until capture is done.
        tft.setCursor(10, screen_height - 10);
        tft.fillRect(10, screen_height-10, 100,10, LA_BG_COLOR);
        tft.setTextColor(LA_TEXT_COLOR);
        tft.printf("Capturing: %lu", myLogicAnalyzer.current_sample_index); // Accessing current_sample_index directly like this is okay if it's volatile and for display only.
        HAL_Delay(50); // Update status text not too frequently
      }
    } else if (current_mode == MODE_SCOPE) {
      // Handle Scope mode (e.g., from previous task)
      // extern Oscilloscope myScope; // If scope object is defined elsewhere
      // myScope.process();
    }
    // Add other mode handling or common tasks

  }
  /* USER CODE END 3 */
}

// Ensure SystemClock_Config(), MX_GPIO_Init(), MX_DMA_Init(), MX_SPI1_Init(), MX_ADC1_Init(), MX_TIM2_Init()
// are correctly defined (typically by STM32CubeMX).
// Ensure `htim2` (TIM_HandleTypeDef for Logic Analyzer) is configured for Update Interrupts.
// Ensure `tft` is initialized before being used by `myLogicAnalyzer`.
// The specific timer instance in `HAL_TIM_PeriodElapsedCallback` (e.g., `htim->Instance == htim2.Instance`)
// must match the timer used for the Logic Analyzer.
// If `tft` is globally defined, ensure it's not multiply defined if other snippets also declare it.
// If `tft` is in `main.c`, then `extern Adafruit_ILI9341 tft;` is correct.

/* End of main.cpp_la_snippet.cpp */
