#include "XPT2046_Touchscreen.h"

// Constructor for bit-banged SPI
XPT2046_Touchscreen::XPT2046_Touchscreen(uint8_t cs_pin, uint8_t irq_pin, uint8_t clk_pin, uint8_t mosi_pin, uint8_t miso_pin) {
  _cs_pin = cs_pin;
  _irq_pin = irq_pin;
  _clk_pin = clk_pin;
  _mosi_pin = mosi_pin;
  _miso_pin = miso_pin;
  _pressure_threshold = 10; // Default pressure threshold, can be adjusted
  _calibrated = false;
}

void XPT2046_Touchscreen::begin() {
  // Initialize pin modes. Our Arduino_STM32_HAL pinMode might be a stub if CubeMX handles it.
  // It's good practice for a library to declare its needs.
  pinMode(_cs_pin, OUTPUT);
  pinMode(_clk_pin, OUTPUT);
  pinMode(_mosi_pin, OUTPUT);
  pinMode(_irq_pin, INPUT); // Or INPUT_PULLUP if IRQ is active low and needs it
  pinMode(_miso_pin, INPUT);

  digitalWrite(_cs_pin, HIGH); // Deselect
  digitalWrite(_clk_pin, LOW); // Clock idles low for common SPI mode used with XPT2046
  digitalWrite(_mosi_pin, LOW);
}

bool XPT2046_Touchscreen::touched() {
  // IRQ pin is usually active low.
  // If IRQ pin is not used or not connected, check pressure from getPoint().
  if (_irq_pin != 255) { // 255 or another value to signify not using IRQ
      return (digitalRead(_irq_pin) == LOW);
  }
  // Fallback to pressure reading if IRQ not used
  TS_Point p = getPoint();
  return (p.z > _pressure_threshold);
}

uint16_t XPT2046_Touchscreen::readData(uint8_t command) {
  uint16_t data = 0;

  digitalWrite(_cs_pin, LOW); // Select chip

  // Send command byte
  spiWrite(command | XPT2046_CTRL_12BIT | XPT2046_CTRL_PD_IRQ); // Use 12-bit mode, power down between conversion, IRQ enabled

  // Read 2 bytes of data (12 bits)
  // XPT2046 typically sends MSB first. The first few bits might be zero.
  // A common timing is to read after the falling edge of CLK.
  // The XPT2046 datasheet specifies data is clocked out on DCLK falling edge,
  // and input is sampled on DCLK rising edge.

  // Send a dummy byte to clock out the first 8 bits of the result
  uint8_t byte1 = spiRead();
  // Send another dummy byte for the next bits
  uint8_t byte2 = spiRead();
  
  digitalWrite(_cs_pin, HIGH); // Deselect chip

  // Combine the bytes into a 12-bit result
  // The XPT2046 typically outputs data in the upper bits.
  // For 12-bit mode, data is often D11..D0.
  // spiRead() returns 8 bits. We need to combine two reads.
  // Command 0xD0 (READ_X): Data is D11..D0
  // Command 0x90 (READ_Y): Data is D11..D0
  // The data comes out MSB first. So byte1 has the MSBs.
  // Example: XPT2046 returns 0bXXXX DDDD DDDD DDDD (where X is don't care, D is data)
  // If spiRead reads MSB of transfer first:
  // byte1 = DDDDDDDD (upper 8 bits of the 12-bit value, shifted if needed)
  // byte2 = DDDDXXXX (lower 4 bits of the 12-bit value, in upper nibble)
  // A common implementation is: data = (spiRead() << 5) | (spiRead() >> 3); for 12 bit result from 16 clocks
  // Or more directly for XPT2046:
  // After sending command, first clock cycle is busy. Then 12 data bits. Then 3 trailing zeros.
  // So, we clock 16 times in total after command.
  // For a simple bit-bang SPI, it often looks like:
  // data = spiTransfer(0x00); data <<= 8; data |= spiTransfer(0x00); data >>= 4; (for 12-bit result in MSBs)

  // Let's use a common interpretation for XPT2046 results:
  // The result is usually spread over 2 bytes, and we need to shift.
  // XPT2046 gives 12 bits. Often D11 is the first bit after busy, D0 is the last.
  data = (byte1 << 8) | byte2;
  data >>= 3; // Shift right by 3 to align 12-bit data (some datasheets show data in bits D14..D3 of a 16-bit frame)
              // Or sometimes data >>= 4; (if data is D15..D4)
              // This depends on the exact XPT2046 variant and interpretation.
              // For URTouch/Stoffregen library, it's often (byte1 << 5 | byte2 >> 3) for 12-bit result
              // This means byte1 = MSB ... bit5, byte2 = bit4 ... LSB XXX
              // Let's stick to a simple version: (read1 << 8 | read2) >> appropriate_shift
              // Assuming data is in bits 14-3 of the 16-bit word read.
  
  // If the controller sends data aligned like this: 000DDDDDDDDDD000 (12 valid bits)
  // then (byte1 << 8 | byte2) would be 000DDDDDDDDDD000.
  // Then data >>= 3; yields 000000DDDDDDDDDD. This is a common way.
  return data;
}

TS_Point XPT2046_Touchscreen::getPoint() {
  uint16_t x_raw, y_raw, z1_raw, z2_raw;
  int16_t z_pressure;

  // XPT2046 requires settling time. A small delay can be useful.
  // Typically, multiple readings are averaged. This is a basic version.
  
  // Power up ADC and reference, but keep IRQ disabled during sequenced reads
  // spiWrite(XPT2046_CTRL_REF_ON | XPT2046_CTRL_12BIT); // Turn on ADC, VREF, 12-bit
  // delayMicroseconds(10); // Allow VREF to settle

  x_raw = readData(XPT2046_CMD_READ_X | XPT2046_CTRL_12BIT | XPT2046_CTRL_PD_IRQ); // Read X, 12-bit, power down
  y_raw = readData(XPT2046_CMD_READ_Y | XPT2046_CTRL_12BIT | XPT2046_CTRL_PD_IRQ); // Read Y, 12-bit, power down
  
  // Pressure reading (optional, can be simplified)
  // For Z (pressure), we need to turn on the X/Y plates.
  // Standard way: CMD_Z1 (0xB0), CMD_Z2 (0xC0)
  // Z = Z1 - Z2 + X_PLATE_RESISTANCE * X_RAW / 4096
  // A simpler pressure reading: just use one Z reading or average.
  // Or use a formula like: z = x_raw * (z2_raw / z1_raw - 1)
  // For now, a basic pressure reading from one of the Z commands:
  z1_raw = readData(XPT2046_CMD_READ_Z1 | XPT2046_CTRL_12BIT | XPT2046_CTRL_PD_IRQ);
  // z2_raw = readData(XPT2046_CMD_READ_Z2 | XPT2046_CTRL_12BIT | XPT2046_CTRL_PD_IRQ);

  // Simple pressure: use z1_raw. Higher value means more pressure.
  // Max value for 12-bit is 4095. A light touch might be a few hundred.
  // Some libraries use a formula: pressure = 4095 - z1_raw (if z1 decreases with pressure)
  // Or pressure = z1_raw (if z1 increases with pressure)
  // Let's assume z1_raw increases with pressure.
  z_pressure = z1_raw;
  if (z_pressure < _pressure_threshold) { // If pressure is too low, invalidate x,y
      x_raw = 0; // Or some other indicator like -1 if TS_Point used signed
      y_raw = 0;
      z_pressure = 0;
  }

  TS_Point p = TS_Point(x_raw, y_raw, z_pressure);
  if (_calibrated) {
    applyCalibration(p);
  }
  return p;
}


// Bit-banged SPI Transfer
uint8_t XPT2046_Touchscreen::spiTransfer(uint8_t data) {
  uint8_t reply = 0;
  for (int i = 7; i >= 0; i--) {
    digitalWrite(_mosi_pin, (data >> i) & 0x01);
    digitalWrite(_clk_pin, HIGH);
    if (digitalRead(_miso_pin) == HIGH) {
      reply |= (1 << i);
    }
    digitalWrite(_clk_pin, LOW);
  }
  return reply;
}

// SPI Write (MOSI only)
void XPT2046_Touchscreen::spiWrite(uint8_t data) {
  for (int i = 7; i >= 0; i--) {
    digitalWrite(_mosi_pin, (data >> i) & 0x01);
    digitalWrite(_clk_pin, HIGH);
    digitalWrite(_clk_pin, LOW);
  }
}

// SPI Read (MISO only, send dummy 0x00 on MOSI)
uint8_t XPT2046_Touchscreen::spiRead() {
  uint8_t reply = 0;
  for (int i = 7; i >= 0; i--) {
    digitalWrite(_clk_pin, HIGH); // Data is usually clocked out by XPT2046 after CLK falling edge
                                 // And sampled by MCU after CLK rising edge (or before falling)
                                 // This depends on SPI mode. XPT2046 is often Mode 0 (CPOL=0, CPHA=0)
                                 // Mode 0: CLK idle low. Data sampled on rising edge, changed on falling.
    // We write MOSI (dummy) then raise CLK, then read MISO, then lower CLK.
    digitalWrite(_mosi_pin, LOW); // Send dummy bit
    //delayMicroseconds(1); // Small delay if needed for timing
    if (digitalRead(_miso_pin) == HIGH) {
      reply |= (1 << i);
    }
    digitalWrite(_clk_pin, LOW);
    //delayMicroseconds(1); // Small delay
  }
  return reply;
}

// --- Calibration ---
void XPT2046_Touchscreen::setCalibration(int16_t x_min, int16_t x_max, int16_t y_min, int16_t y_max, 
                                         uint16_t screen_width, uint16_t screen_height, bool rotate) {
    _x_min_raw = x_min;
    _x_max_raw = x_max;
    _y_min_raw = y_min;
    _y_max_raw = y_max;
    _screen_width = screen_width;
    _screen_height = screen_height;
    _rotate_touch = rotate; // If true, swap x/y and invert one axis for rotated display
    _calibrated = true;
}

void XPT2046_Touchscreen::applyCalibration(TS_Point &p) {
    if (!_calibrated || p.z < _pressure_threshold) { // Don't map if not touched or not calibrated
        p.x = -1; // Indicate invalid point if using signed, or map to 0,0
        p.y = -1;
        return;
    }

    // Clamp raw values to calibration range
    p.x = (p.x < _x_min_raw) ? _x_min_raw : p.x;
    p.x = (p.x > _x_max_raw) ? _x_max_raw : p.x;
    p.y = (p.y < _y_min_raw) ? _y_min_raw : p.y;
    p.y = (p.y > _y_max_raw) ? _y_max_raw : p.y;

    int32_t screen_x, screen_y;

    // Map to screen coordinates
    screen_x = (int32_t)(p.x - _x_min_raw) * _screen_width  / (_x_max_raw - _x_min_raw);
    screen_y = (int32_t)(p.y - _y_min_raw) * _screen_height / (_y_max_raw - _y_min_raw);

    // Handle potential screen orientation (inversion of axis)
    // This depends on how the touch panel is mounted relative to the display
    // For example, if raw X increases from left-to-right, but screen X is also left-to-right:
    // screen_x = screen_x;
    // If raw Y increases from top-to-bottom, but screen Y is also top-to-bottom:
    // screen_y = screen_y;
    
    // Example: if touch Y is inverted compared to screen Y
    // screen_y = _screen_height - screen_y;

    if (_rotate_touch) { // Apply rotation (e.g. 90 degrees for landscape display)
        int16_t temp = screen_x;
        screen_x = screen_y;
        screen_y = _screen_width - temp; // Example for 90 deg rotation on 240x320 -> 320x240
                                         // Max new Y is old max X (_screen_width)
    }
    
    p.x = screen_x;
    p.y = screen_y;
}

/*
Example Calibration Values (these are placeholders, must be found for each setup)
// For a 240x320 display
#define XPT2046_MIN_RAW_X 200
#define XPT2046_MAX_RAW_X 3800
#define XPT2046_MIN_RAW_Y 250
#define XPT2046_MAX_RAW_Y 3750
// ts.setCalibration(XPT2046_MIN_RAW_X, XPT2046_MAX_RAW_X, XPT2046_MIN_RAW_Y, XPT2046_MAX_RAW_Y, 320, 240, true); // For landscape
*/
