#ifndef XPT2046_Touchscreen_h
#define XPT2046_Touchscreen_h

// Use our STM32 HAL compatibility layer
#include "Arduino_STM32_HAL.h" // Provides digitalWrite, digitalRead, pinMode, delayMicroseconds

// Data class for returning touch coordinates
class TS_Point {
public:
  TS_Point(void) : x(0), y(0), z(0) {}
  TS_Point(int16_t x, int16_t y, int16_t z) : x(x), y(y), z(z) {}
  bool operator==(TS_Point p) { return ((p.x == x) && (p.y == y) && (p.z == z)); }
  bool operator!=(TS_Point p) { return ((p.x != x) || (p.y != y) || (p.z != z)); }
  int16_t x, y, z;
};

// Define commands for the XPT2046
#define XPT2046_CMD_READ_X  0xD0 // Or 0x90 for 12-bit, 0xD0 for 8-bit differential
#define XPT2046_CMD_READ_Y  0x90 // Or 0xA0 for 12-bit, 0x90 for 8-bit differential
#define XPT2046_CMD_READ_Z1 0xB0 // Pressure measurement
#define XPT2046_CMD_READ_Z2 0xC0 // Pressure measurement
#define XPT2046_CTRL_12BIT  0x00 // 12-bit conversion mode
#define XPT2046_CTRL_8BIT   0x08 // 8-bit conversion mode
#define XPT2046_CTRL_PD_IRQ 0x00 // Power Down between conversions, IRQ enabled
#define XPT2046_CTRL_PD_NOIRQ 0x01 // Power Down, IRQ disabled
#define XPT2046_CTRL_ADC_ON 0x02 // ADC on, IRQ disabled
#define XPT2046_CTRL_REF_ON 0x03 // ADC on, VREF on, IRQ disabled

class XPT2046_Touchscreen {
public:
  // Constructor for bit-banged SPI
  XPT2046_Touchscreen(uint8_t cs_pin, uint8_t irq_pin, uint8_t clk_pin, uint8_t mosi_pin, uint8_t miso_pin);

  void begin();
  bool touched();
  TS_Point getPoint();

  // Raw data reading function
  uint16_t readData(uint8_t command);

  // Calibration parameters (optional, can be set by user)
  void setCalibration(int16_t x_min, int16_t x_max, int16_t y_min, int16_t y_max, uint16_t screen_width, uint16_t screen_height, bool rotate);
  void applyCalibration(TS_Point &p);


private:
  uint8_t _cs_pin;
  uint8_t _irq_pin;
  uint8_t _clk_pin;
  uint8_t _mosi_pin;
  uint8_t _miso_pin;

  uint16_t _pressure_threshold; // Threshold for touch detection

  // SPI bit-bang transfer
  uint8_t spiTransfer(uint8_t data);
  void spiWrite(uint8_t data);
  uint8_t spiRead();

  // Calibration related
  bool _calibrated;
  int16_t _x_min_raw, _x_max_raw, _y_min_raw, _y_max_raw;
  uint16_t _screen_width, _screen_height;
  bool _rotate_touch; // if touch coordinates should be rotated
};

#endif // XPT2046_Touchscreen_h
