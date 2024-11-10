/*
 * sdcard.c
 *
 *  Created on: Nov 5, 2024
 *      Author: rob
 */
#include "ch.h"
#include "hal.h"
#include "portab.h"
#include "ff.h"

/*===========================================================================*/
/* Card insertion monitor.                                                   */
/*===========================================================================*/

#define POLLING_INTERVAL                10
#define POLLING_DELAY                   10
bool fs_ready = FALSE;
bool autorun = FALSE;
FATFS SDC_FS;
extern const MMCConfig portab_mmccfg;
extern MMCDriver MMCD1;

/**
 * @brief   Card monitor timer.
 */
static virtual_timer_t tmr;

/**
 * @brief   Debounce counter.
 */
static unsigned cnt;

/**
 * @brief   Card event sources.
 */
event_source_t inserted_event, removed_event; //, autorun_terminated;

/**
 * @brief   Insertion monitor timer callback function.
 *
 * @param[in] p         pointer to the @p BaseBlockDevice object
 *
 * @notapi
 */
static void tmrfunc(void *p) {
  BaseBlockDevice *bbdp = p;

  chSysLockFromISR();
  if (cnt > 0) {
    if (blkIsInserted(bbdp)) {
      if (--cnt == 0) {
        chEvtBroadcastI(&inserted_event);
      }
    }
    else
      cnt = POLLING_INTERVAL;
  }
  else {
    if (!blkIsInserted(bbdp)) {
      cnt = POLLING_INTERVAL;
      chEvtBroadcastI(&removed_event);
    }
  }
  chVTSetI(&tmr, TIME_MS2I(POLLING_DELAY), tmrfunc, bbdp);
  chSysUnlockFromISR();
}

/**
 * @brief   Polling monitor start.
 *
 * @param[in] p         pointer to an object implementing @p BaseBlockDevice
 *
 * @notapi
 */
static void tmr_init(void *p) {

  chEvtObjectInit(&inserted_event);
  chEvtObjectInit(&removed_event);
  chSysLock();
  cnt = POLLING_INTERVAL;
  chVTSetI(&tmr, TIME_MS2I(POLLING_DELAY), tmrfunc, p);
  chSysUnlock();
}

/*
 * Card insertion event.
 */
void InsertHandler(eventid_t id) {
  FRESULT err;
  (void)id;
  /*
   * On insertion SDC initialization and FS mount.
   */
  if (mmcConnect(&MMCD1)){
    return;
  }
  err = f_mount(&SDC_FS, "/", 1);
  if (err != FR_OK) {
    mmcDisconnect(&MMCD1);
    return;
  }
  fs_ready = TRUE;
  autorun = TRUE;
  chSysLock();
  //chThdResumeI(&artp, MSG_OK);
  chSysUnlock();
}

/*
 * Card removal event.
 */
void RemoveHandler(eventid_t id) {

  (void)id;
  mmcDisconnect(&MMCD1);
  fs_ready = FALSE;
}


void sdcard_init(void){
  palSetPadMode(GPIOB, 12, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST); // CS SD Card
  palSetPadMode(GPIOA, 8, PAL_MODE_INPUT_PULLUP);    // Detect Card
  palSetPadMode(GPIOB, 13, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST); // SPI SD Card SCK
  palSetPadMode(GPIOB, 14, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST); // SPI SD Card MISO
  palSetPadMode(GPIOB, 15, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST); // SPI SD Card MOSI
  palSetPad(GPIOB, 12);

  /* MMC/SD over SPI driver configuration.*/
  // static MMCConfig mmccfg = {&SPID2, &ls_spicfg, &hs_spicfg};

  mmcObjectInit(&MMCD1);
  mmcStart(&MMCD1, &portab_mmccfg);

  /* Activates the card insertion monitor.*/
  tmr_init(&MMCD1);
}
