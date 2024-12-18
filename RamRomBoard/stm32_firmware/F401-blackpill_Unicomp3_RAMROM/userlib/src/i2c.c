/*
 * i2c.c
 *
 *  Created on: Sep 15, 2023
 *      Author: rob
 */

#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "i2c.h"
#include "upload.h"
#include "portab.h"
#include <string.h>

static const I2CConfig i2cfg1 = {
    OPMODE_I2C,
    400000,
    FAST_DUTY_CYCLE_2,
};

extern BaseSequentialStream *const chout;
extern BaseSequentialStream *const dbg;

/*
 *******************************************************************************
 * INTERNAL FUNCTIONS
 *******************************************************************************
 */

static systime_t calc_timeout(I2CDriver *i2cp, size_t txbytes, size_t rxbytes){
  const uint32_t bitsinbyte = 10;
  uint32_t tmo;
  tmo = ((txbytes + rxbytes + 1) * bitsinbyte * 1000);
  tmo /= i2cp->config->clock_speed;
  tmo += 5; /* some additional time to be safer */
  //return MS2ST(tmo);
  return TIME_MS2I(tmo);
}

void WriteDev(uint8_t * txbuf, uint8_t txbytes){
  msg_t status = MSG_OK;
  uint8_t rxbuf;
  //sysinterval_t tmo = TIME_MS2I(4);
  sysinterval_t tmo = calc_timeout(&I2C_BUS, txbytes, 0);
  //i2cAcquireBus(&I2C_BUS);
  status = i2cMasterTransmitTimeout(&I2C_BUS, DS1085_DEVADDR, txbuf, txbytes, &rxbuf, 0, tmo);
  //i2cReleaseBus(&I2C_BUS);
  osalDbgCheck(MSG_OK == status);
}

void ReadDev(uint8_t* txbuf, uint8_t* rxbuf, uint8_t txbytes, uint8_t rxbytes){
  msg_t status = MSG_OK;
  sysinterval_t tmo = calc_timeout(&I2C_BUS, txbytes, rxbytes);
  //i2cAcquireBus(&I2C_BUS);
  status = i2cMasterTransmitTimeout(&I2C_BUS, DS1085_DEVADDR, txbuf, txbytes, rxbuf, rxbytes, tmo);
  //i2cReleaseBus(&I2C_BUS);
  osalDbgCheck(MSG_OK == status);
}

uint8_t get_range(void){
  uint8_t tx = DS1085_RANGE;
  uint8_t temp[2];
  ReadDev(&tx, temp, 1, 2);
  return temp[0]>>3;
}

uint8_t get_offset(void){
  uint8_t tx = DS1085_OFFSET;
  uint8_t temp;
  ReadDev(&tx, &temp, 1, 1);
  return (temp&0x1F);
}

void set_offset(uint8_t val){
    uint8_t temp[2];
    temp[0] = DS1085_OFFSET;
    temp[1] = val & 0x1F;
    WriteDev(temp, 2);
}

uint8_t get_addr(void){
  uint8_t tx = DS1085_ADDR;
  uint8_t temp;
  ReadDev(&tx, &temp, 1, 1);
  return (temp&0x0F);
}

void set_address(uint8_t val){
    uint8_t temp[2];
    temp[0] = DS1085_ADDR;
    temp[1] = val & 0x0F;
    WriteDev(temp, 2);
}

uint16_t get_two_byte(uint8_t reg){  // good for DAC, MUX and DIV Register
  uint8_t tx = reg;
  uint8_t temp[2];
  uint16_t retval;
  ReadDev(&tx, temp, 1, 2);
  if (DEBUGLEVEL == 5){
    chprintf(dbg, "RVal: %02d, %02X, %02X\r\n", reg, temp[0], temp[1]);
  }
  retval = temp[1]>>6;
  retval |= (uint16_t)(temp[0]<<2);
  return retval;
}

void set_two_byte(uint8_t reg, uint16_t val){  // good for DAC, MUX and DIV Register
    uint8_t tx[3];
    union { uint16_t x; uint8_t b[2]; } conv;
    conv.x = val << 6;
    tx[0] = reg;
    tx[1] = conv.b[1];
    tx[2] = conv.b[0];
   if (DEBUGLEVEL == 5){
    chprintf(dbg, "TVal: %02d, %02X, %02X\r\n", tx[0], tx[1], tx[2]);
    }
    WriteDev(tx, 3);
}

uint8_t read_clock(uint8_t* buf){
  uint16_t temp;
  buf[0] = get_range();
  buf[1] = get_offset();
  buf[2] = get_addr();
  temp = get_two_byte(DS1085_MUX);
  buf[4] = (uint8_t)(temp & 0xFF);
  buf[3] = (uint8_t)(temp >> 8);
  temp = get_two_byte(DS1085_DAC);
  buf[6] = (uint8_t)(temp & 0xFF);
  buf[5] = (uint8_t)(temp >> 8);
  temp = get_two_byte(DS1085_DIV);
  buf[8] = (uint8_t)(temp & 0xFF);
  buf[7] = (uint8_t)(temp >> 8);
  if (DEBUGLEVEL == 5){
    chprintf(dbg, "Clock (Rd): %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X %02X\r\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8]);
  }
  return 9;
}

void write_clock(uint8_t* buf){
 uint8_t oldval[9];
 read_clock(oldval);
 if (DEBUGLEVEL == 5){
    chprintf(dbg, "Old:       %02d, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X\r\n", oldval[0], oldval[1], oldval[2], oldval[3], oldval[4], oldval[5], oldval[6], oldval[7], oldval[8]);
}
 if ((buf[1]  != oldval[1]) ) set_offset(buf[1]);
 if ((buf[2]  != oldval[2]) ) set_address(buf[2]);
 if ((buf[3]  != oldval[3]) || (buf[4] != oldval[4]) ) set_two_byte(DS1085_MUX, (uint16_t)((buf[3] << 8) | buf[4]));
 if ((buf[5]  != oldval[5]) || (buf[6] != oldval[6]) ) set_two_byte(DS1085_DAC, (uint16_t)((buf[5] << 8) | buf[6]));
 if ((buf[7]  != oldval[7]) || (buf[8] != oldval[8]) ) set_two_byte(DS1085_DIV, (uint16_t)((buf[7] << 8) | buf[8]));
 if (DEBUGLEVEL == 5){
    chprintf(dbg, "New:       %02d, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X\r\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8]);
 }
}

void i2c_init(void){
  //palSetPadMode(GPIOB, 6, PAL_MODE_STM32_ALTERNATE_PUSHPULL);
  palSetPadMode(GPIOB, 6, (PAL_MODE_ALTERNATE(4) | PAL_STM32_OTYPE_OPENDRAIN));
  //palSetPadMode(GPIOB, 7, PAL_MODE_STM32_ALTERNATE_PUSHPULL);
  palSetPadMode(GPIOB, 7, (PAL_MODE_ALTERNATE(4) | PAL_STM32_OTYPE_OPENDRAIN));
  palSetLineMode(CTRL1, PAL_MODE_OUTPUT_PUSHPULL);
  //OUTPUT_OFF;
  OUTPUT_ON;
  i2cStart(&I2C_BUS, &i2cfg1);
  i2cAcquireBus(&I2C_BUS);
}
