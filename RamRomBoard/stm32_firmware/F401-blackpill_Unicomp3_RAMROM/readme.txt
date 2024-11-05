*****************************************************************************
** ChibiOS/RT port for ARM-Cortex-M4 STM32F407.                            **
*****************************************************************************

** TARGET **

The demo runs on an STM32F411 or STM32F401 blackpill board.
(25MHz Crystal)
Serial Port over USB (also Debug Interface).
Serial 115200 Bd

Build:
Build with eclipse or simply type "make" in the code folder.


Functions:

Pinout:
PA0  - JTAG - TDO
PA1  - /PLD (Parallel Load into Shift Register)
PA2  - TX2 (Console + Debug)
PA3  - RX2 (Console + Debug)
PA4  - /BW (Battery Warning out or Chip Enable in)
PA5  - SCK1
PA6  - MISO1
PA7  - MOSI1
PA8  - DET_SD (SD Card Detect)
PA9  - /STM32BUS
PA10 - /EXTRESET (Reset Target)
PA11 - USD DM (Ostrich)
PA12 - USB DP (Ostrich)
PA13 - SWDIO
PA14 - SWCLK
PA15 - CNT

PB0  - /CEDAT_EN (Enable Data for CE RAM)
PB1  - /MRAM_WR (Write Pulse for Main RAM)
PB2  - /DATOE (Data Output Enable from Main RAM)
PB3  - /MRC (Clear Address Counter)
PB4  - CLK1 - PWM Output (TIM3/1) (Clock 1Hz)
PB5  - /CNTOE (Output Enable for Address Counter)
PB6  - SCL (used for Clock Chip DS1085)
PB7  - SDA (used for Clock Chip DS1085)
PB8  - CTRL1 (CTRL1 for clock chip)
PB9  - /BUSFREE (signal if bus is free)
PB10 - TRST (Target Reset)

PB12 - /CS_SD (SD Card Chipselect)
PB13 - SCK_SD
PB14 - MISO_SD
PB15 - MOSI_SD

PC13 - JTAG - TDI
PC14 - JTAG - TMS
PC15 - JTAG - TCK


