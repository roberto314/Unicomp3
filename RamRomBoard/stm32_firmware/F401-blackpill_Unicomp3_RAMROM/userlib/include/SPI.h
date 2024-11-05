/*
 * SPI.h
 *
 *  Created on: Nov 19, 2022
 *      Author: rob
 */

#ifndef USERLIB_INCLUDE_SPI_H_
#define USERLIB_INCLUDE_SPI_H_

#include "ch.h"
#include "hal.h"

//typedef union {
//  uint32_t _32;
//  struct _8{
//    uint8_t _8l;
//    uint8_t _8lm;
//    uint8_t _8hm;
//    uint8_t _8h;
//  };
//}u_cv_t;

typedef struct {
    union {
      uint32_t _32;
      struct {
        uint8_t _8l;
        uint8_t _8lm;
        uint8_t _8hm;
        uint8_t _8h;
      };
    };
  uint8_t cs;
  uint8_t mask;
} st_configdata_t;

#define SPI_DRIVER   (&SPID1)
#define SPI_PORT     GPIOA
#define SCK_PAD      5  //PA5
#define MISO_PAD     6  //PA6
#define MOSI_PAD     7  //PA7
//#define CS_PORT      GPIOC
//#define CS_PAD       4

#define DEBUG         PAL_LINE(GPIOC, 15U) // Is JTAG Clock
#define LED           PAL_LINE(GPIOC, 13U) // Is JTAG Clock

#define PLD           PAL_LINE(GPIOA, 1U)  // Low: Load into Shift register
#define RAMCE         PAL_LINE(GPIOA, 4U)  // Low: RAM Chip Enable
#define EXTRST        PAL_LINE(GPIOA, 10U) // External reset
#define CPC           PAL_LINE(GPIOA, 15U) // Counter Tic

#define CERAMWR       PAL_LINE(GPIOB, 0U)
#define RAMWR         PAL_LINE(GPIOB, 1U)  // RAM Writepulse
#define DATOE         PAL_LINE(GPIOB, 2U)  // RAM Output Enable
#define MRC           PAL_LINE(GPIOB, 3U)  // Counter Reset - Low Active
#define CNTOE         PAL_LINE(GPIOB, 5U)   // Counter Output Enable - Low Active
#define BUSFREE       PAL_LINE(GPIOB, 9U)  // Low if Bus is High-Z - Input
#define TRESET        PAL_LINE(GPIOB, 10U) // Target Reset - High active

#define DEBUG_HI  palSetLine(DEBUG)
#define DEBUG_LOW palClearLine(DEBUG)
#define LED_OFF  palSetLine(LED)
#define LED_ON palClearLine(LED)
#define RAM_ACTIVE palClearLine(RAMCE)
#define RAM_INACTIVE palSetLine(RAMCE)
#define PLD_IDLE palSetLine(PLD)
#define PLD_LOAD palClearLine(PLD)
#define CPC_HIGH palSetLine(CPC)
#define CPC_LOW palClearLine(CPC)
#define CEWR_INACTIVE palSetLine(CERAMWR)
#define CEWR_ACTIVE palClearLine(CERAMWR)
#define WE_INACTIVE palSetLine(RAMWR)
#define WE_ACTIVE palClearLine(RAMWR)
#define DATOE_ACTIVE palClearLine(DATOE)
#define DATOE_INACTIVE palSetLine(DATOE)
#define MRC_INACTIVE palSetLine(MRC)
#define MRC_ACTIVE palClearLine(MRC)
#define CNTOE_INACTIVE palSetLine(CNTOE)
#define CNTOE_ACTIVE palClearLine(CNTOE)
#define TRESET_INACTIVE palClearLine(TRESET)
#define TRESET_ACTIVE palSetLine(TRESET)

void SPI_init(void);
//void WriteSPI(int32_t val);
void write_single_byte(uint8_t data, int32_t address, uint8_t reset);
uint8_t read_single_byte(int32_t address, uint8_t reset);
uint8_t read_next_byte(void);
void write_next_byte(uint8_t data);
void read_block(int32_t address, int32_t len, uint8_t * data, uint8_t reset);
void write_block(int32_t address, int32_t len, uint8_t * data, uint8_t reset);
void write_config(uint8_t *buf);
void write_pins(uint8_t data);

#endif /* USERLIB_INCLUDE_SPI_H_ */
