/*
 * SPI.c
 *
 *  Created on: Nov 19, 2022
 *      Author: rob
 */

#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "SPI.h"
#include "upload.h"
#include "portab.h"
#include "i2c.h"

extern BaseSequentialStream *const chout;
//extern st_configdata_t cfdat[20];
extern BaseSequentialStream *const dbg;
volatile uint8_t BUS_in_use;

// SPI setup ajust " SPI_BaudRatePrescaler_X" to set SPI speed.
// Peripherial Clock 42MHz SPI2 SPI3
// Peripherial Clock 84MHz SPI1                                SPI1        SPI2/3
#define SPI_BaudRatePrescaler_2         ((uint16_t)0x0000) //  42 MHz      21 MHZ
#define SPI_BaudRatePrescaler_4         ((uint16_t)0x0008) //  21 MHz      10.5 MHz
#define SPI_BaudRatePrescaler_8         ((uint16_t)0x0010) //  10.5 MHz    5.25 MHz
#define SPI_BaudRatePrescaler_16        ((uint16_t)0x0018) //  5.25 MHz    2.626 MHz
#define SPI_BaudRatePrescaler_32        ((uint16_t)0x0020) //  2.626 MHz   1.3125 MHz
#define SPI_BaudRatePrescaler_64        ((uint16_t)0x0028) //  1.3125 MHz  656.25 KHz
#define SPI_BaudRatePrescaler_128       ((uint16_t)0x0030) //  656.25 KHz  328.125 KHz
#define SPI_BaudRatePrescaler_256       ((uint16_t)0x0038) //  328.125 KHz 164.06 KHz
static SPIConfig spi_cfg_8 = { //Config for 8bits
  FALSE,
  NULL,
  NULL,
  0,
  SPI_BaudRatePrescaler_4 ,// CPOL = 0| SPI_CR1_CPOL, // CPOL = 1
  0
};

static void gptcb(GPTDriver *drv){
  (void)drv;
  BUS_in_use = 0;
  //DEBUG_HI;
  //palToggleLine(DEBUG);
  //gptStopTimerI(drv);
}
/*
 * GPT6 configuration.
 */
static const GPTConfig gptcfg1 = {
  .frequency    = 1000000U, //1MHz Timer Freq.
  .callback     = gptcb,
  .cr2          = 0U,
  .dier         = 0U
};

static void write_byte(uint8_t data);
static uint8_t read_byte(void);

static void check_BUS(void){
  /* Check if BUS is used at all. If not there is a timer callback after 10us
      which sets the variable BUS_in_use to 0 . */
  //DEBUG_LOW;
  BUS_in_use = 1;
  gptStartContinuous(&GPTD4, 10U);
  if (DEBUGLEVEL == 4) chprintf(dbg, "waiting for BUSFREE to go high.\r\n");
  while ((palReadLine(BUSFREE) == PAL_LOW) && BUS_in_use == 1);
  if (DEBUGLEVEL == 4) chprintf(dbg, "waiting for BUSFREE to go low.\r\n");
  while ((palReadLine(BUSFREE) == PAL_HIGH) && BUS_in_use == 1);
  if (DEBUGLEVEL == 4){
      if (BUS_in_use == 0){
        chprintf(dbg, "BUSFREE is staying Low.\r\n");
      }
      else {
        chprintf(dbg, "BUSFREE is changing.\r\n");
      }
  }
  gptStopTimer(&GPTD4);
}

static inline void wait_for_busfree(void){
  if (BUS_in_use == 0){
    return;
  }
  while (palReadLine(BUSFREE) == PAL_LOW);
  while (palReadLine(BUSFREE) == PAL_HIGH);
}

static void increment_address(void){
  CPC_HIGH; // Count up
  __NOP();
  CPC_LOW;  // Latch into Output Register
  __NOP();
}

void setup_address(int32_t address){
  int32_t i;
  // HACK! Somehow address 0 won't get read correctly unless there are some 
  // wiggles of the CPC Line during reset
  MRC_ACTIVE;  // Reset '590
  __NOP();
  CPC_HIGH; // Count up
  __NOP();
  CPC_LOW;  // Latch into Output Register
  __NOP();
  CPC_HIGH; // Count up
  __NOP();
  CPC_LOW;  // Latch into Output Register
  __NOP();
  MRC_INACTIVE;
  __NOP();
  CPC_HIGH; // Count up
  __NOP();
  CPC_LOW;  // Latch into Output Register
  __NOP();
  for (i=0; i<address; i++){
    increment_address();
  }
  return;
}

static void write_byte(uint8_t data){
  uint8_t buf[1];
  buf[0] = data;
  spiSend(SPI_DRIVER, 1, buf);
  chSysLock();
  wait_for_busfree(); // this adds about 130ns after falling edge of BUSFREE
  CNTOE_ACTIVE; // needs ~ 25ns for the Address to become stable
  __NOP();
  __NOP();
  WE_ACTIVE;
  __NOP();
  __NOP();
  WE_INACTIVE;
  __NOP();
 // CNTOE_INACTIVE; //DEBUG
  chSysUnlock();
}

static uint8_t read_byte(void){
  uint8_t ret;
  DEBUG_LOW;
  chSysLock();
  wait_for_busfree(); // this adds about 130ns after falling edge of BUSFREE
  DEBUG_HI;
  CNTOE_ACTIVE; // needs ~ 25ns for the Address to become stable
  __NOP();
  __NOP();
  DATOE_ACTIVE; // needs ~ 10ns for the Data to become stable
  __NOP();
  PLD_LOAD;
  __NOP();
  PLD_IDLE;
  DATOE_INACTIVE;
  __NOP();
 //CNTOE_INACTIVE; //DEBUG
  chSysUnlock();
  spiReceive(SPI_DRIVER, 1, &ret);
  return ret;
}

void write_single_byte(uint8_t data, int32_t address, uint8_t reset){
  setup_address(address);
  check_BUS();
  if (reset){
    TRESET_ACTIVE;
    BUS_in_use = 0;
  }
  write_byte(data);
  if (reset){
    TRESET_INACTIVE;
    BUS_in_use = 1;
  }
}

uint8_t read_single_byte(int32_t address, uint8_t reset){
  uint8_t data = 0;
  setup_address(address);
  check_BUS();
  if (reset){
    TRESET_ACTIVE;
    BUS_in_use = 0;
  }
  data = read_byte();
  if (reset){
    TRESET_INACTIVE;
    BUS_in_use = 1;
  }
  return data;
}

uint8_t read_next_byte(void){
  uint8_t data = 0;
  increment_address();
  __NOP();
  data = read_byte();
  return data;
}

void write_next_byte(uint8_t data){
  increment_address();
  __NOP();
  write_byte(data);
}

void read_block(int32_t address, int32_t len, uint8_t * data, uint8_t reset){
  int32_t l = len;
  setup_address(address);
  check_BUS();
  if (reset){
    TRESET_ACTIVE;
    BUS_in_use = 0;
  }
  while(l--){
    *data++ = read_byte();
    increment_address();
  }
  if (reset){
    TRESET_INACTIVE;
    BUS_in_use = 1;
  }
}

void write_block(int32_t address, int32_t len, uint8_t * data, uint8_t reset){
  int32_t l = len;
  //select_chip(15);
  setup_address(address);
  check_BUS();
  if (reset){
    TRESET_ACTIVE;
    BUS_in_use = 0;
  }
  while(l--){
    write_byte(*data++);
    increment_address();
  }
  if (reset){
    TRESET_INACTIVE;
    BUS_in_use = 1;
  }
}

void write_block_no_setup(int32_t len, uint8_t * data, uint8_t reset){
  int32_t l = len;
  check_BUS();
  if (reset){
    TRESET_ACTIVE;
    BUS_in_use = 0;
  }
  while(l--){
    write_byte(*data++);
    increment_address();
  }
  if (reset){
    TRESET_INACTIVE;
    BUS_in_use = 1;
  }
}

static void write_int(uint16_t data){
  uint8_t buf[2];
  buf[0] = (uint8_t)(data >> 8) & 0xFF;
  buf[1] = (uint8_t)(data);
  spiSend(SPI_DRIVER, 2, buf);
  chSysLock();
  CEWR_ACTIVE;
  __NOP();
  __NOP();
  CEWR_INACTIVE;
  chSysUnlock();
}

void write_csdata(uint32_t start, uint32_t end, uint16_t data){
  uint32_t pos = start;
  //chprintf(dbg, "Address: 0x%06X, CS: 0x%04X\r\n", pos, data);
  while (pos++ <= end){
    write_int(data);
    increment_address();
  }
}

//void fill_struct(uint8_t* in, st_configdata_t* out){
//  out->_8h  = 0;
//  out->_8hm = *in++;
//  out->_8lm = *in++;
//  out->_8l  = *in++;
//  out->cs   = *in++;
//  out->mask = *in++;
//}
//
//#define ADDRESS (cfdat._32)
//#define NEXTADDRESS (nextcf._32)
//#define CHIP (cfdat.cs)
//#define MASK (cfdat.mask)

void write_config(uint8_t* buf, uint32_t size){
  uint32_t i, saddress, eaddress;
  uint16_t csdata1, csdata2;
  setup_address(0);
  TRESET_ACTIVE; // for security
  CNTOE_ACTIVE;
  for (i=0; i<size; i+=10){
    saddress = buf[i]*65536;
    saddress += buf[i+1]*256;
    saddress += buf[i+2];
    csdata1 = buf[i+3]*256;
    csdata1 += buf[i+4];
    eaddress = buf[i+5]*65536;
    eaddress += buf[i+6]*256;
    eaddress += buf[i+7];
    csdata2 = buf[i+8]*256;
    csdata2 += buf[i+9];
    if (csdata1 == csdata2){
      write_csdata(saddress, eaddress, csdata1);
      if (DEBUGLEVEL == 4) chprintf(dbg, "D: %d, Address: 0x%06X - 0x%06X, CS: 0x%04X\r\n", i, saddress, eaddress, csdata1);
    }
    else{
      chprintf(dbg, "ERROR!, CSDATA doesn't match. %04X %04X", csdata1, csdata2); 
    }
  }
  CNTOE_INACTIVE;
  TRESET_INACTIVE;
}

void write_pins(uint8_t data){
  switch (data){
  case 0:
    chprintf(dbg, "in Reset inactive... \r\n");
    TRESET_INACTIVE;
    break;
  case 1:
    chprintf(dbg, "in Reset active... \r\n");
    TRESET_ACTIVE;
    break;
  case 2:
    chprintf(dbg, "in Reset onoff... \r\n");
    TRESET_ACTIVE;
    chThdSleepMilliseconds(100);
    TRESET_INACTIVE;
    break;
  default:
    chprintf(dbg, "in Reset unhandled... \r\n");
    break;
  }
}

void SPI_init(void){
  palSetPadMode(SPI_PORT, SCK_PAD, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);
  palSetPadMode(SPI_PORT, MOSI_PAD, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);
  palSetPadMode(SPI_PORT, MISO_PAD, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);

  PLD_IDLE;
  RAM_INACTIVE;
  CPC_HIGH;
  CEWR_INACTIVE;
  WE_INACTIVE;
  DATOE_INACTIVE;
  MRC_INACTIVE;
  CNTOE_INACTIVE;
  TRESET_INACTIVE;

  DEBUG_HI;
  LED_OFF;
  palSetLineMode(DEBUG, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);
  palSetLineMode(LED, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);

  palSetLineMode(BUSFREE, PAL_MODE_INPUT);
  
  palSetLineMode(PLD, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);
  palSetLineMode(CERAMWR, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);
  palSetLineMode(RAMCE, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);
  palSetLineMode(CPC, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);
  palSetLineMode(RAMWR, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);
  palSetLineMode(DATOE, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);
  palSetLineMode(MRC, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);
  palSetLineMode(CNTOE, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);
  palSetLineMode(TRESET, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);
  
  spiStart(SPI_DRIVER, &spi_cfg_8);
  spiAcquireBus(SPI_DRIVER);
  /*
   * Starting GPT5 driver, it is used for checking the BUSFREE Signal
   */
  gptStart(&GPTD4, &gptcfg1);

  BUS_in_use = 1; //assume bus is in use
  RAM_ACTIVE; // activate RAM
}
