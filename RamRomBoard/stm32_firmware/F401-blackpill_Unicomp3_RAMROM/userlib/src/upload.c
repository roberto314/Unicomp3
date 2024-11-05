/*
 * upload.c
 *
 *  Created on: Dec 1, 2022
 *      Author: rob
 */

#include <stdio.h>
#include <string.h>
#include "portab.h"
#include "ch.h"
#include "hal.h"
#include "portab.h"
#include "upload.h"
#include "chprintf.h"
#include "usbcfg.h"
#include "SPI.h"
#include "i2c.h"

extern BaseSequentialStream *const ost;
extern BaseSequentialStream *const dbg;
static BUFFER_LONG_ST long_buf;
static BUFFER_SHORT_ST short_buf;

void debug_print_state(char * text, uint8_t val){
  if (DEBUGLEVEL >= 3){
    switch (val){
    case IDLE:
      chprintf(dbg, "%s", text);
      chprintf(dbg, "IDLE\r\n");
      break;
    case START_H:
      chprintf(dbg, "%s", text);
      chprintf(dbg, "START_H\r\n");
      break;
    case START_M:
      chprintf(dbg, "%s", text);
      chprintf(dbg, "START_M;\r\n");
      break;
    case START_L:
      chprintf(dbg, "%s", text);
      chprintf(dbg, "START_L\r\n");
      break;
    case LEN_H:
      chprintf(dbg, "%s", text);
      chprintf(dbg, "LEN_H\r\n");
      break;
    case LEN_M:
      chprintf(dbg, "%s", text);
      chprintf(dbg, "LEN_M;\r\n");
      break;
    case LEN_L:
      chprintf(dbg, "%s", text);
      chprintf(dbg, "LEN_L\r\n");
      break;
    case IDX_L:
      chprintf(dbg, "%s", text);
      chprintf(dbg, "IDX_L\r\n");
      break;
    case IDX_S:
      chprintf(dbg, "%s", text);
      chprintf(dbg, "IDX_S\r\n");
      break;
    case DATA_L:
      chprintf(dbg, "%s", text);
      chprintf(dbg, "DATA_L\r\n");
      break;
    case DATA_S:
      chprintf(dbg, "%s", text);
      chprintf(dbg, "DATA_S\r\n");
      break;
    case CHECKSUM_L:
      chprintf(dbg, "%s", text);
      chprintf(dbg, "CHECKSUM_L\r\n");
      break;
    case CHECKSUM_S:
      chprintf(dbg, "%s", text);
      chprintf(dbg, "CHECKSUM_S\r\n");
      break;
    default:
      chprintf(dbg, "%s", text);
      chprintf(dbg, "UNHANDLED\r\n");
      break;
    }
  }
}

void debug_print_val1(char * text, uint16_t val){
  if (DEBUGLEVEL >= 2){
    chprintf(dbg, "%s %04x\r\n", text, val);
  }
}

void send_buffer(uint8_t *cs, uint8_t *buf, uint32_t len){
  uint32_t i;
  for (i=0; i<len; i++){
    *cs += buf[i];
    streamPut(ost, buf[i]);
  }
}

void send_three_byte(uint8_t *cs, uint32_t val){
  uint8_t buf[3], i;
  buf[0] = (uint8_t)((val&0x00FF0000) >> 16);
  buf[1] = (uint8_t)((val&0x0000FF00) >> 8);
  buf[2] = (uint8_t)((val&0x000000FF));
  for (i=0; i<3; i++){
    *cs += buf[i];
    streamPut(ost, buf[i]);
  }
}

void send_two_byte(uint8_t *cs, uint16_t val){
  uint8_t buf[2], i;
  buf[0] = (uint8_t)((val&0xFF00) >> 8);
  buf[1] = (uint8_t)((val&0x00FF));
  for (i=0; i<2; i++){
    *cs += buf[i];
    streamPut(ost, buf[i]);
  }
}

void send_one_byte(uint8_t *cs, uint8_t val){
  uint8_t buf;
  buf = (uint8_t)(val);
  *cs += buf;
  streamPut(ost, buf);
}

void handle_short_buf(void){
  uint8_t cs, temp = 0;
  cs = 0;
  switch (short_buf.func){
  case RAMR:
  	if (short_buf.tindex == 0){
  		cs = read_single_byte(short_buf.tstart, 0);
  		short_buf.tsize--;
  		streamPut(ost, cs);
  	}
	while (short_buf.tsize--){ 
    	temp = read_next_byte();
		cs += temp;
		streamPut(ost, temp);
		//chThdSleepMilliseconds(1);
	}
    streamPut(ost, cs);
  	break;
  case CLOCKR:
  	read_clock(short_buf.tbuf); // 9 bytes
  	send_buffer(&cs, short_buf.tbuf, 9);
    streamPut(ost, cs);
  	break;
  case VERSION:
    //chprintf(dbg, "in Version, returning bytes... \r\n");
  	send_one_byte(&cs, 'U');
  	send_one_byte(&cs, 'C');
  	send_one_byte(&cs, '3');
  	send_one_byte(&cs, VMAJOR);
  	send_one_byte(&cs, VMINOR);
    streamPut(ost, cs);
  	break;
  case PROGRESS:
    chprintf(dbg, "Progress %d \r\n", short_buf.tsize);

    streamPut(ost, cs);
    break;
  case STATUS:
    //chprintf(dbg, "Status %d %d %d\r\n", long_buf.bytes_written, long_buf.bsize[0], long_buf.bsize[1]);
    send_three_byte(&cs, long_buf.bytes_written);
    send_two_byte(&cs, long_buf.bsize[0]);
    send_two_byte(&cs, long_buf.bsize[1]);
    streamPut(ost, cs);
    break;
  case TRESETW:
  	temp = short_buf.tbuf[0];
  	write_pins(temp);
    streamPut(ost, 'Y');
	break;
  case CLOCKW:
  	write_clock(short_buf.tbuf);
    streamPut(ost, 'Y');
	break;
  default:
    break;
  }
}

static THD_WORKING_AREA(waCharacterInputThread, 256);
static THD_FUNCTION(CharacterInputThread, arg) {
  uint8_t c;
  static uint32_t cnt_idx, count, chunk_cnt, start_address;
  uint16_t func;
  char_state_t state = IDLE;
  systime_t start, end;
  static uint8_t cs, idx;

  (void)arg;
  while (true){
#ifdef OSTRICHUSB
    if (OSTRICHPORT.config->usbp->state == USB_ACTIVE) {
#else
    if (1){
#endif
      c=streamGet(&OSTRICHPORT);
      start = chVTGetSystemTime();

      if (start > end){
        state = IDLE;
      }
      end = chTimeAddX(start, TIME_MS2I(500));
      //sdAsynchronousRead(&OSTRICHPORT, (uint8_t *)&c, 1);
      if (state == IDLE){
        debug_print_state("------------ State0: ------------ ", state);
      }

      switch (state){
        //#######################  FUNCTION ##########################
        case IDLE:
          cs = c;
          cnt_idx = 0;
          //end = chTimeAddX(chVTGetSystemTimeX(), TIME_MS2I(5));
          state = START_H;
          func = c;
          debug_print_val1("Func: ", c);
          debug_print_state("To State ", state);
          break;
        //#######################  START_H ##########################
        case START_H:
          cs += c;
          state = START_M;
          start_address = (uint32_t)c * 65536;
          debug_print_val1("Start High: ", c);
          debug_print_state("To State: ", state);
          break;
        //#######################  START_M ##########################
        case START_M:
          cs += c;
          state = START_L;
          start_address += (uint32_t)c * 256;
          debug_print_val1("Start Med: ", c);
          debug_print_state("To State: ", state);
          break;
        //#######################  START_L ##########################
        case START_L:
          cs += c;
          state = LEN_H;
          start_address += (uint32_t)c;
          debug_print_val1("Start Low: ", c);
          debug_print_state("To State: ", state);
          break;
        //#######################  LEN_H ##########################
        case LEN_H:
          cs += c;
          state = LEN_M;
          count = (uint32_t)c * 65536;
          debug_print_val1("Len High: ", c);
          debug_print_state("To State: ", state);
          break;
        //#######################  LEN_M ##########################
        case LEN_M:
          cs += c;
          state = LEN_L;
          count += (uint32_t)c * 256;
          debug_print_val1("Len Med: ", c);
          debug_print_state("To State: ", state);
          break;
        //#######################  LEN_L ##########################
        case LEN_L:
          cs += c;
          count += (uint32_t)c;
          state = IDX_S;                // below 128 are short buffer functions
          if ((func > 128) && (func < 192)) state = IDX_L; // 129-191 are long buffer functions
          debug_print_val1("Len Low: ", c);
          debug_print_state("To State: ", state);
          break;
        //#################### Index SHORT #######################
        case IDX_S:
          cs += c;
          if (func < 128){ // Below 128 are Read Functions (no data)
            state = CHECKSUM_S;
          }
          else{
            state = DATA_S;
          }
          idx = c;
//          if (idx == 0){
//
//          }
          debug_print_val1("Index: ", c);
          debug_print_state("To State: ", state);
          break;
        //###################### Index LONG ######################
        case IDX_L:
          cs += c;
          state = DATA_L;
          idx = c;
          if (idx == 0){
            long_buf.bytes_written = 0;
            long_buf.tsize = count; // This is the total size of the upload
            long_buf.tstart = start_address; // Start Address for RAM
          }
          chunk_cnt = BUFFSIZE; // usually we fill the buffer full
          if (long_buf.tsize < BUFFSIZE){ // If we have only one chunk
            chunk_cnt = long_buf.tsize;
          }
          else{                         // we have more than one chunk
            if ((long_buf.tsize - ((idx) * BUFFSIZE)) >= BUFFSIZE){ // Rest is >=  BUFFSIZE
              chunk_cnt = BUFFSIZE; // Buffer full
            }
            else{
              chunk_cnt = (long_buf.tsize - ((idx) * BUFFSIZE)); // Buffer partially full
            }
          }
          debug_print_val1("Chunk index: ", idx);
          debug_print_val1("Chunk Size: ", chunk_cnt);
          debug_print_state("To State: ", state);
          break;
        //###################### Data LONG #########################
        case DATA_L:
          cs += c;
          long_buf.tbuf[(BUFFSIZE * (idx & 1)) + cnt_idx++] = c;

          if (cnt_idx == chunk_cnt){
            state = CHECKSUM_L;
            debug_print_val1("Offset: ", (BUFFSIZE * (idx & 1)));
            debug_print_state("To State: ", state);
          }          
          break;
        //###################### Data SHORT #########################
        case DATA_S:
          cs += c;
          short_buf.tbuf[cnt_idx++] = c;

          if (cnt_idx == count){
            state = CHECKSUM_S;
            debug_print_val1("Offset: ", (BUFFSIZE * (idx & 1)));
            debug_print_state("To State: ", state);
          }          
          break;
        //##################### Checksum LONG ########################
        case CHECKSUM_L:
          state = IDLE;
          if (c == cs){
            //debug_print_val1("Checksum OK: ", cs);
            long_buf.index = idx;
            long_buf.bsize[(idx & 1)] = chunk_cnt;
            chprintf(dbg, "BufferSize: 0: %d 1: %d, Index: %d\r\n", long_buf.bsize[0], long_buf.bsize[1], idx);
            //if (DEBUGLEVEL >= 1){
            //  chprintf(dbg, "XSVF (C): cnt: %03d, data: %02X, %02X, %02X, %02X\r\n", count, tbuf1[0], tbuf1[1], tbuf1[2], tbuf1[3]);
            //}
            if (long_buf.bsize[0] && long_buf.bsize[1]){
              chprintf(ost, "W"); // Checksum OK, Buffer full.
              chprintf(dbg, "Buffer Full, Wake Thread up.\r\n");
              //chThdResume(&writetp, func);
            }
            else{
              chprintf(ost, "O"); // Checksum OK.
            }
            //chprintf(dbg, "Back.\r\n");
            //if (write_xsvf(count, long_buf.bufp) == 0) chprintf(ost, "X"); // Checksum or Programming Error
          }
          else{
            chprintf(ost, "X"); // Checksum or Programming Error
            chprintf(dbg, "Checksum ERROR L\r\n");
          }
          break;          
        //##################### Checksum SHORT ########################
        case CHECKSUM_S:
          state = IDLE;
          if (c == cs){
            debug_print_val1("Checksum OK: ", cs);
            short_buf.tindex = idx;
            short_buf.tsize = count;
            short_buf.func = func;
            short_buf.tstart = start_address;
            chprintf(ost, "O"); // Checksum OK.
            handle_short_buf();
          }
          else{
            chprintf(ost, "X"); // Checksum or Programming Error (resend)
            chprintf(dbg, "Checksum ERROR S\r\n");
          }
          break;          
        //####################### WRITE ##########################

      case UNHANDLED:
        state = IDLE;
        break;
      }
    }
    else{
      chThdSleepMilliseconds(100);
    }
  }
}

void start_upload_thread(void){
  long_buf.working = 0;
  long_buf.bsize[0] = 0;
  long_buf.bsize[1] = 0;	
  chThdCreateStatic(waCharacterInputThread, sizeof(waCharacterInputThread), NORMALPRIO, CharacterInputThread, NULL);
}
