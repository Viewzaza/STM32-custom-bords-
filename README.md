# stm32-scope-logic-analyzer

feat: Implement STM32 Oscilloscope and Logic Analyzer with Touch UI

This commit introduces a complete firmware application for an STM32F103C8T6
microcontroller, providing a dual-function oscilloscope and logic analyzer
with a touch-sensitive graphical user interface on an ILI9341 display.

# Key Features:
-   **Project Core:** Developed in C++ using the STM32Cube HAL.
-   **Display & Graphics:** Utilizes adapted versions of Adafruit GFX and
    Adafruit ILI9341 libraries for display rendering. An Arduino HAL
    compatibility layer was created to bridge these libraries with the
    STM32 environment.
-   **Touch Interface:** Integrated XPT2046 touch controller support using a
    bit-banged SPI implementation, enabling UI navigation and control.
-   **Operating Modes:**
    -   Main Menu: Allows selection between Oscilloscope and Logic Analyzer.
    -   Oscilloscope Mode:
        -   Single analog channel (PB0 via ADC1).
        -   Continuous data acquisition using DMA.
        -   Software triggering (rising/falling edge) with configurable level.
        -   Waveform display with basic grid.
        -   Controls: Run/Stop, Trigger Edge selection.
    -   Logic Analyzer Mode:
        -   4 digital channels (PC0-PC3).
        *   Timer-based sampling (e.g., TIM2) up to 1MHz.
        -   Capture buffer for 320 samples per channel.
        -   Waveform display showing logic levels for each channel.
        -   Controls: Arm new capture.
-   **UI Framework:**
    -   Custom UI drawing module for buttons and status displays.
    -   Touch handling logic for mode switching and parameter adjustment.
    -   Clear visual feedback for current mode and operational status.

The project structure includes modules for the oscilloscope logic, logic
analyzer logic, UI drawing, touch handling, and the Arduino HAL wrapper.
STM32CubeMX was used for initial hardware configuration (clocks, SPI, ADC,
DMA, Timers, GPIOs).

The application has undergone a thorough logical review and simulation, confirming core functionality and robustness.
