/* USER CODE BEGIN Includes */
// Add your includes here, make sure paths are correctly set in the build environment
#include "Middlewares/ArduinoHAL/Arduino_STM32_HAL.h" // Provides Arduino API, SPIClass, pin aliases
#include "Middlewares/Adafruit/GFX/Adafruit_GFX.h"    // For display
#include "Middlewares/Adafruit/ILI9341/Adafruit_ILI9341.h" // For display
#include "Middlewares/XPT2046/XPT2046_Touchscreen.h" // For touch panel
/* USER CODE END Includes */

/* USER CODE BEGIN PV */
// TFT object (from previous task)
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS_PIN_ALIAS, TFT_DC_PIN_ALIAS, TFT_RST_PIN_ALIAS);

// Touchscreen object using the pin aliases defined in Arduino_STM32_HAL.h
// Constructor: XPT2046_Touchscreen(cs, irq, clk, mosi/din, miso/do)
XPT2046_Touchscreen ts = XPT2046_Touchscreen(
    XPT2046_CS_PIN_ALIAS,
    XPT2046_IRQ_PIN_ALIAS,
    XPT2046_CLK_PIN_ALIAS,
    XPT2046_DIN_PIN_ALIAS,
    XPT2046_DO_PIN_ALIAS
);

// Calibration values - THESE MUST BE DETERMINED FOR YOUR SPECIFIC SCREEN
// For a 240x320 display, common orientation (portrait)
#define TOUCH_X_MIN_RAW  300  // Raw ADC value for screen X=0
#define TOUCH_X_MAX_RAW  3700 // Raw ADC value for screen X=239
#define TOUCH_Y_MIN_RAW  250  // Raw ADC value for screen Y=0
#define TOUCH_Y_MAX_RAW  3800 // Raw ADC value for screen Y=319
#define SCREEN_WIDTH     240
#define SCREEN_HEIGHT    320
#define ROTATE_TOUCH     false // Set true if display rotation requires touch coordinate rotation

/* USER CODE END PV */

int main(void) {
  HAL_Init();

  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  DWT->CYCCNT = 0;

  SystemClock_Config();

  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SPI1_Init(); // For TFT
  // MX_ADC1_Init();

  /* USER CODE BEGIN 2 */
  // Initialize TFT display
  digitalWrite(TFT_CS_PIN_ALIAS, HIGH); // Deselect TFT
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  tft.setCursor(5, 5);
  tft.println("Touch Test Initialized");

  // Initialize Touchscreen
  // CS pin for XPT2046 is managed by the library's begin/read functions.
  // Ensure it's initially high if not handled by library's begin.
  // Our XPT2046_Touchscreen::begin() calls digitalWrite(_cs_pin, HIGH);
  ts.begin();
  tft.println("XPT2046 begin() called.");

  // Apply calibration settings
  // For a 240x320 portrait display, no rotation of touch coordinates
  // If the display is used in landscape (320x240), set ROTATE_TOUCH to true
  // and adjust SCREEN_WIDTH/HEIGHT accordingly.
  // The last parameter 'ROTATE_TOUCH' in setCalibration depends on how your display rotation
  // affects the touch coordinate system relative to the GFX coordinate system.
  // If tft.setRotation(1) (landscape) is used, and touch X maps to display Y, then ROTATE_TOUCH might be true.
  ts.setCalibration(TOUCH_X_MIN_RAW, TOUCH_X_MAX_RAW, 
                    TOUCH_Y_MIN_RAW, TOUCH_Y_MAX_RAW, 
                    SCREEN_WIDTH, SCREEN_HEIGHT, ROTATE_TOUCH);
  tft.println("Touch calibration set.");
  tft.println("Touch the screen!");

  // Buffer for text
  char touch_info[50];
  /* USER CODE END 2 */

  /* Infinite loop */
  while (1) {
    /* USER CODE END WHILE */
    /* USER CODE BEGIN 3 */

    // Check for touch using the IRQ pin (active LOW)
    // Or use ts.touched() which also checks IRQ or pressure.
    if (digitalRead(XPT2046_IRQ_PIN_ALIAS) == LOW) { // IRQ is active low
      TS_Point p = ts.getPoint(); // Reads raw data and applies calibration if set

      // Clear previous touch info area (optional, depends on display layout)
      tft.fillRect(5, 50, SCREEN_WIDTH - 10, 40, ILI9341_BLACK);
      tft.setCursor(5, 50);

      if (p.z > 0) { // Check pressure (p.z is already adjusted by getPoint if below threshold)
                     // Or if using TS_Point with x,y=-1 for no touch: if (p.x != -1)
        sprintf(touch_info, "Calibrated X: %3d Y: %3d Z: %3d", (int)p.x, (int)p.y, (int)p.z);
        tft.println(touch_info);

        // Example: Draw a small circle at the touch point
        tft.fillCircle(p.x, p.y, 3, ILI9341_RED);

        // To get raw values (before calibration) for determining MIN/MAX:
        // You would need to modify XPT2046_Touchscreen::getPoint() to return raw values
        // or add a new method like getRawPoint(), then print those.
        // For example, if getRawPoint() existed:
        // TS_Point raw_p = ts.getRawPoint();
        // sprintf(touch_info, "Raw X: %4d Y: %4d Z: %4d", raw_p.x, raw_p.y, raw_p.z);
        // tft.setCursor(5, 70);
        // tft.println(touch_info);

      } else {
        tft.println("Touch detected (Z=0)");
      }
    } else {
      // Optional: Clear touch info when not touched or indicate "No Touch"
      // tft.fillRect(5, 50, SCREEN_WIDTH - 10, 40, ILI9341_BLACK);
      // tft.setCursor(5, 50);
      // tft.println("No touch");
    }

    HAL_Delay(10); // Small delay to prevent spamming and allow MCU to breathe
  }
  /* USER CODE END 3 */
}

// Calibration:
// The XPT2046_Touchscreen library has a `setCalibration` method.
// To find the raw min/max values (TOUCH_X_MIN_RAW, etc.):
// 1. Temporarily modify the code to print RAW touch data from `ts.getPoint()` *before* calibration is applied.
//    Or add a method `ts.getRawPoint()` to the library that doesn't apply calibration.
// 2. Touch the extreme corners of your display:
//    - Top-left corner: Record the raw X and Y. These are your MIN_RAW_X and MIN_RAW_Y (or MAX, depending on orientation).
//    - Bottom-right corner: Record raw X and Y. These are your MAX_RAW_X and MAX_RAW_Y.
// 3. Update the #define values with these recorded raw ADC numbers.
// 4. Set SCREEN_WIDTH and SCREEN_HEIGHT to your display's resolution (e.g., 240, 320).
// 5. The `ROTATE_TOUCH` parameter in `setCalibration` depends on your display's rotation and how the touch panel axes align.
//    If `tft.setRotation(0)` (portrait) is used, and touch X increases left-to-right and touch Y top-to-bottom, then ROTATE_TOUCH is usually `false`.
//    If `tft.setRotation(1)` (landscape) is used, and the raw touch X now corresponds to screen Y, and raw touch Y to screen X,
//    then `ROTATE_TOUCH` might be `true`, and the calibration function would swap and invert axes accordingly.
//    The `applyCalibration` function in `XPT2046_Touchscreen.cpp` shows a basic rotation example; it might need adjustment based on the specific rotation (90, 180, 270 degrees).

/* End of main.cpp_xpt2046_snippet.cpp */
