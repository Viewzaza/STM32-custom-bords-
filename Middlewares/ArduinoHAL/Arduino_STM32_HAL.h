#ifndef ARDUINO_STM32_HAL_H
#define ARDUINO_STM32_HAL_H

#include "stm32f1xx_hal.h" // Specific to STM32F1 series
#include <stdint.h>
#include <stddef.h> // For size_t

// Basic Arduino data types
typedef uint8_t boolean;
typedef uint8_t byte;

#define HIGH 1
#define LOW  0

#define INPUT             0x00
#define OUTPUT            0x01
#define INPUT_PULLUP      0x02 // Note: STM32 HAL has separate PULLUP/PULLDOWN config

// Define aliases for pins used by the TFT display.
// These are arbitrary numbers that the Adafruit_ILI9341 constructor will receive.
// Our digitalWrite function will then map these to actual STM32 pins.
#define TFT_CS_PIN_ALIAS  10 // Example: PA1 for ILI9341_CS
#define TFT_DC_PIN_ALIAS  9  // Example: PA0 for ILI9341_DC/RS
#define TFT_RST_PIN_ALIAS 8  // Example: PA2 for ILI9341_RESET

// XPT2046 Pin Aliases (for bit-banged SPI)
#define XPT2046_CS_PIN_ALIAS   7 // PB13
#define XPT2046_IRQ_PIN_ALIAS  6 // PA8
#define XPT2046_CLK_PIN_ALIAS  5 // PB12
#define XPT2046_DIN_PIN_ALIAS  4 // MOSI, maps to PB14
#define XPT2046_DO_PIN_ALIAS   3 // MISO, maps to PB15


// STM32 Pin Definitions corresponding to aliases (used internally in .cpp)
// Ensure these match the actual hardware connections and CubeMX configuration.
// For ILI9341 (Display)
#define ILI9341_CS_PORT     GPIOA
#define ILI9341_CS_PIN      GPIO_PIN_1
#define ILI9341_DC_PORT     GPIOA
#define ILI9341_DC_PIN      GPIO_PIN_0
#define ILI9341_RST_PORT    GPIOA
#define ILI9341_RST_PIN     GPIO_PIN_2

// For XPT2046 (Touch Panel)
#define XPT2046_CS_PORT     GPIOB
#define XPT2046_CS_PIN      GPIO_PIN_13
#define XPT2046_IRQ_PORT    GPIOA
#define XPT2046_IRQ_PIN     GPIO_PIN_8
#define XPT2046_CLK_PORT    GPIOB
#define XPT2046_CLK_PIN     GPIO_PIN_12
#define XPT2046_DIN_PORT    GPIOB // MOSI
#define XPT2046_DIN_PIN     GPIO_PIN_14
#define XPT2046_DO_PORT     GPIOB // MISO
#define XPT2046_DO_PIN      GPIO_PIN_15


// SPI Modes (from Arduino SPI.h)
#define SPI_MODE0 0x00
#define SPI_MODE1 0x04
#define SPI_MODE2 0x08
#define SPI_MODE3 0x0C

// Bit order
#define LSBFIRST 0
#define MSBFIRST 1

// Function Declarations
void pinMode(uint16_t pin_alias, uint8_t mode); // Mode is INPUT, OUTPUT etc.
void digitalWrite(uint16_t pin_alias, uint8_t val);
int digitalRead(uint16_t pin_alias); // STM32 HAL returns GPIO_PinState (0 or 1)
void delay(uint32_t ms);
void delayMicroseconds(uint32_t us);

// Simple SPISettings class (as used by Adafruit libraries)
class SPISettings {
public:
  SPISettings(uint32_t clock, uint8_t bitOrder, uint8_t dataMode) :
    _clock(clock), _bitOrder(bitOrder), _dataMode(dataMode) {}
  SPISettings() : _clock(4000000), _bitOrder(MSBFIRST), _dataMode(SPI_MODE0) {} // Default

  uint32_t _clock;
  uint8_t _bitOrder;
  uint8_t _dataMode;
};

// Basic SPIClass
class SPIClass {
public:
  SPIClass(SPI_HandleTypeDef* hspi); // Constructor to pass the HAL SPI handle
  void begin();
  uint8_t transfer(uint8_t data);
  void transfer(void *buf, size_t count); // For block transfers
  void beginTransaction(SPISettings settings);
  void endTransaction(void);

  // Helper to set the SPI handle, used if a global SPI object needs late init
  void setHandle(SPI_HandleTypeDef* hspi);

private:
  SPI_HandleTypeDef* _hspi; // Pointer to the HAL SPI handle (e.g., &hspi1)
  SPISettings _currentSettings;
};

extern SPI_HandleTypeDef hspi1; // Assume hspi1 is defined in main.c or spi.c
extern SPIClass SPI; // Declare the global SPI object

#endif // ARDUINO_STM32_HAL_H
