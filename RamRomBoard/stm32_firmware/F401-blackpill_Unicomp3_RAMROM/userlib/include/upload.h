/*
 * upload.h
 *
 *  Created on: Nov 3, 2024
 *      Author: rob
 */

#ifndef USERLIB_INCLUDE_UPLOAD_H_
#define USERLIB_INCLUDE_UPLOAD_H_

//#define BUFFSIZE 16384
#define BUFFSIZE 8192
#define BUFFMASK ((BUFFSIZE*2)-1)
typedef enum {
  IDLE = 0,
  START_H = 1,
  START_M,
  START_L,
  LEN_H,
  LEN_M,
  LEN_L,
  IDX_L,
  IDX_S,
  DATA_L,
  DATA_S,
  CHECKSUM_L,
  CHECKSUM_S,
  UNHANDLED
} char_state_t;

/* From 0 to 127 are read Functions,
 * 128 is unhandled, from 129 to 191 are long buffer write functions
 * from 192 to 255 are short buffer write functions.
 */
typedef enum {
	RAMR = 123,
	CLOCKR = 124,
  	VERSION = 125,
  	STATUS = 126,
  	PROGRESS = 127,
  	NO_FUNC = 128,
  	XSVF = 129,
  	RAMW = 130,
  	TRESETW = 192,
  	CLOCKW = 193,
} function_t;

typedef struct {
  function_t func;
  uint8_t working;
  uint32_t bytes_written;
  uint8_t index;
  uint16_t bsize[2];
  uint32_t tsize;
  uint32_t tstart;
  uint8_t tbuf[2*BUFFSIZE];
} BUFFER_LONG_ST;

typedef struct {
  function_t func;
  uint32_t tsize;
  uint32_t tstart;
  uint8_t tindex;
  uint8_t tbuf[256];
} BUFFER_SHORT_ST;

void start_upload_thread(void);

#endif /* USERLIB_INCLUDE_UPLOAD_H_ */
