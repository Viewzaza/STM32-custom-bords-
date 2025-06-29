/* USER CODE BEGIN Includes */
// Add your includes here, make sure paths are correctly set in the build environment
#include "Middlewares/ArduinoHAL/Arduino_STM32_HAL.h" // Provides Arduino API, SPIClass, and pin aliases
#include "Middlewares/Adafruit/GFX/Adafruit_GFX.h"
#include "Middlewares/Adafruit/ILI9341/Adafruit_ILI9341.h"
/* USER CODE END Includes */

/* USER CODE BEGIN PV */
// Define the TFT object using the pin aliases from Arduino_STM32_HAL.h
// These aliases (TFT_CS_PIN_ALIAS, etc.) are mapped to actual STM32 pins
// inside the Arduino_STM32_HAL.cpp digitalWrite function.
// The SPI pins (MOSI, MISO, SCK) are handled by the SPIClass using the hspi1 peripheral.
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS_PIN_ALIAS, TFT_DC_PIN_ALIAS, TFT_RST_PIN_ALIAS);
/* USER CODE END PV */

/*
 * Remember to have these global variables defined in your main.c/main.cpp or relevant HAL init file (e.g. spi.c)
 * if they are not already:
 * SPI_HandleTypeDef hspi1; // This should be generated by CubeMX in spi.c or main.c
 */


int main(void) {
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  // Enable DWT Cycle Counter for delayMicroseconds, if not already enabled by HAL_Init or SystemClock_Config
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  DWT->CYCCNT = 0; // Reset counter
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config(); // Assuming this is generated by CubeMX

  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();   // Generated by CubeMX
  MX_DMA_Init();    // Generated by CubeMX (if used for other purposes)
  MX_SPI1_Init();   // Generated by CubeMX - vital for TFT communication
  // MX_ADC1_Init(); // etc. for other peripherals

  /* USER CODE BEGIN 2 */
  // Initialize Chip Select for TFT to HIGH (inactive)
  // Our digitalWrite uses the aliases to control the correct STM32 pins
  digitalWrite(TFT_CS_PIN_ALIAS, HIGH);

  // Initialize the display
  tft.begin(); // This calls SPI.begin() internally if needed, and sends init commands

  // Test graphics
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(10, 10);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.println("Hello STM32!");

  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(1);
  tft.setCursor(10, 50);
  tft.println("Adafruit GFX on STM32 HAL");

  tft.drawCircle(tft.width()/2, tft.height()/2 + 20, 20, ILI9341_RED);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    // Example: Blink an LED or update display content
    HAL_Delay(1000);
  }
  /* USER CODE END 3 */
}

// SystemClock_Config() and peripheral init functions (MX_GPIO_Init, etc.)
// are assumed to be defined elsewhere, typically generated by STM32CubeMX.
// Make sure that the hspi1 handle (for SPI1) is correctly initialized
// and that the SPIClass in Arduino_STM32_HAL.cpp uses its address.
// The global `SPI` object is initialized in `Arduino_STM32_HAL.cpp` as `SPIClass SPI(&hspi1);`
// so it should automatically pick up the correct SPI peripheral.

// Ensure include paths for Middlewares are added to the project's C/C++ build settings:
// - Middlewares/ArduinoHAL
// - Middlewares/Adafruit/GFX
// - Middlewares/Adafruit/ILI9341

// Ensure all necessary .cpp and .c files are compiled:
// - Middlewares/ArduinoHAL/Arduino_STM32_HAL.cpp
// - Middlewares/Adafruit/GFX/Adafruit_GFX.cpp
// - Middlewares/Adafruit/GFX/Adafruit_SPITFT.cpp
// - Middlewares/Adafruit/GFX/glcdfont.c
// - Middlewares/Adafruit/ILI9341/Adafruit_ILI9341.cpp
// (And any other dependencies like Adafruit_GrayOLED.cpp if used by GFX for certain features)

// The `glcdfont.c` file from Adafruit GFX library provides the `font` array.
// The `Arduino_STM32_HAL.cpp` has `extern const unsigned char font[] PROGMEM;`
// which should link against the definition in `glcdfont.c`.
// The `PROGMEM` macros are defined in `Arduino_STM32_HAL.h` to be compatible
// with ARM architecture (i.e., `const` and direct pointer access).

// Key check for SPISettings in Arduino_STM32_HAL.cpp:
// The Adafruit_ILI9341 library might call SPI.beginTransaction with specific SPI settings.
// The current implementation of SPIClass::beginTransaction in Arduino_STM32_HAL.cpp
// attempts to reconfigure HAL SPI if mode or bit order changes.
// The default ILI9341 communication uses SPI_MODE0 and MSBFIRST.
// The clock speed is often passed as a parameter to tft.begin(speed) or set in SPISettings.
// My `beginTransaction` doesn't fully support dynamic clock speed changes via prescaler adjustment yet,
// it assumes the CubeMX configured speed is generally used or that the library
// uses a speed compatible with the default prescaler.
// The ILI9341 constructor does not take SPI settings; it uses default ones or tft.begin() might set them.
// Adafruit_SPITFT.h defines `DEFAULT_SPI_FREQ 12000000`. Our SPI is at 4.5MHz.
// The `tft.begin()` may try to set this, so `SPI.beginTransaction` needs to be robust or
// the library's requested speed needs to be ignored/adapted if it's too high for current prescaler.
// For now, the `beginTransaction` in the HAL wrapper is basic.
// The `hspi1` is configured for 4.5 Mbaud (APB2=72MHz, Prescaler=16).
// Adafruit_ILI9341 typically uses `SPI_MODE0`. This matches `CPOL=LOW, CPHA=1EDGE` for STM32.

// If `Adafruit_SPITFT.cpp` has `SPI.setClockDivider(SPI_CLOCK_DIV2);` or similar,
// those are Arduino AVR specific and need to be mapped to `SPISettings` or handled.
// Modern Adafruit libraries usually use `SPI.beginTransaction`.
// The `Adafruit_SPITFT` constructor takes an `spi_dev` argument that can be `&SPI`
// or it defaults to using the global `SPI` object. Our setup relies on the global `SPI`.
// The `_freq` member in `Adafruit_SPITFT` is initialized, and used in its `beginTransaction`.
// Our `SPIClass::beginTransaction` must handle the `settings._clock` parameter,
// at least by logging or ensuring it doesn't try to re-init SPI unnecessarily if the mode is the same.
// The current `beginTransaction` only re-inits if CPOL/CPHA/BitOrder change.

/* End of main.cpp_snippet.cpp */
