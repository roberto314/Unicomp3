/*
    ChibiOS - Copyright (C) 2006..2018 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/
#include "main.h"
#include "ch.h"
#include "hal.h"
#include <string.h>
#include <stdlib.h>
#include "portab.h"
#include "shell.h"
#include "chprintf.h"

#include "SPI.h"
#include "i2c.h"
#include "usbcfg.h"
#include "upload.h"
#include "comm.h"
#include "mypwm.h"

//#define usb_lld_connect_bus(usbp)
//#define usb_lld_disconnect_bus(usbp)

extern THD_WORKING_AREA(waCharacterInputThread, 128);
extern THD_FUNCTION(CharacterInputThread, arg);

/* Function prototypes needed as the two callbacks call each other.
   there is no way to order the callback without triggering an error. */
static void button_cb(void *arg);
static void vt_cb(virtual_timer_t *vtp, void *p);
static uint8_t is_pressed = 0, btn_cnt = 0;
/*===========================================================================*/
/* VT related code.                                                          */
/*===========================================================================*/
/* Virtual timer. */
static virtual_timer_t vt, vt2;
/* Callback of the virtual timer. */
static void vt_cb(virtual_timer_t *vtp, void *p) {
  (void)vtp;
  (void)p;
  chSysLockFromISR();
  /* Enabling the event and associating the callback. */
  if (is_pressed)
    palEnableLineEventI(EXTRST, PAL_EVENT_MODE_RISING_EDGE);
  else
    palEnableLineEventI(EXTRST, PAL_EVENT_MODE_FALLING_EDGE);
  palSetLineCallbackI(EXTRST, button_cb, NULL);
  chSysUnlockFromISR();
}

static void vt2_cb(virtual_timer_t *vtp, void *p) {
  (void)vtp;
  (void)p;
  chSysLockFromISR();
  if (btn_cnt < 2){
    LED_OFF;
  }
  btn_cnt = 0;
  chSysUnlockFromISR();
}
/*===========================================================================*/
/* Button related code.                                                      */
/*===========================================================================*/
/* Callback associated to the raising edge of the button line. */
static void button_cb(void *arg) {
  (void)arg;
  //palToggleLine(LED);
  if (is_pressed){
    is_pressed = 0;
    btn_cnt++;
  }
  else{
    is_pressed = 1;
    LED_ON;
  }
  chSysLockFromISR();
  /* Disabling the event on the line and setting a timer to
     re-enable it. */
  palDisableLineEventI(EXTRST);
  /* Arming the VT timer to re-enable the event in 50ms. */
  chVTResetI(&vt);
  chVTDoSetI(&vt, TIME_MS2I(50), vt_cb, NULL);
  chVTResetI(&vt2);
  chVTDoSetI(&vt2, TIME_MS2I(300), vt2_cb, NULL);
  chSysUnlockFromISR();
}
/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/
BaseSequentialStream *const shell = (BaseSequentialStream *)&SHELLPORT;
BaseSequentialStream *const ost = (BaseSequentialStream *)&OSTRICHPORT;
BaseSequentialStream *const dbg = (BaseSequentialStream *)&DEBUGPORT;

#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(2048)

char history_buffer[8*64];
char *completion_buffer[SHELL_MAX_COMPLETIONS];

static const ShellCommand commands[] = {
  {"xhelp",cmd_xhelp},
  {"rb", cmd_rb},
  {"wb", cmd_wb},
  {"fill", cmd_fill},
  {"br", cmd_br},
  {"freq", cmd_change_freq},
  {"dc", cmd_change_pw4},
  {"spi",cmd_spi},
  {"wc", cmd_wc},
  {"test",cmd_test},
  {NULL, NULL}
};
static const ShellConfig shell_cfg1 = {
  (BaseSequentialStream *)&SHELLPORT,
  commands,
  history_buffer,
  sizeof(history_buffer),
  completion_buffer
};

//static const ShellCommand commands[] = {
//  {"test", cmd_test},
//  {NULL, NULL}
//};
//
//static const ShellConfig shell_cfg1 = {
//  (BaseSequentialStream *)&SHELLPORT,
//  commands
//};

/*
 * Green LED blinker thread, times are in milliseconds.
 */
//static THD_WORKING_AREA(waThread1, 128);
//static THD_FUNCTION(Thread1, arg) {
//
//  (void)arg;
//  chRegSetThreadName("blinker");
//  while (true) {
//    //palClearPad(GPIOA, 6);
//    chThdSleepMilliseconds(500);
//    //palSetPad(GPIOA, 6);
//    chThdSleepMilliseconds(500);
//  }
//}

/*
 * Application entry point.
 */
int main(void) {
  thread_t *shelltp = NULL;
  event_listener_t shell_el;

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  //PA2(TX) and PA3(RX) are routed to USART2
  SerialConfig serial_config6 = {
      //115200,
      115200,
      0,
      0,
      0
  };

  sdStart(&SHELLPORT, &serial_config6);
  palSetPadMode(GPIOA, 2, PAL_MODE_ALTERNATE(7));
  palSetPadMode(GPIOA, 3, PAL_MODE_ALTERNATE(7));

  chprintf(dbg, "\r\nUnicomp RAMROM: %i.%i \r\nSystem started. (Shell)\r\n", VMAJOR, VMINOR);
  //chprintf(ost, "\r\nNVRAM Programmer: %i.%i \r\nSystem started. (Ostrich)\r\nTest with 'VV' - should return 'N'", VMAJOR, VMINOR);

//  #ifdef OSTRICHUSB
  sduObjectInit(&OSTRICHPORT);
  sduStart(&OSTRICHPORT, &serusbcfg1);
//  palSetPadMode(GPIOA, 11, PAL_MODE_ALTERNATE(10));
//  palSetPadMode(GPIOA, 12, PAL_MODE_ALTERNATE(10));

  usbDisconnectBus(serusbcfg1.usbp);
  chThdSleepMilliseconds(1500);
  usbStart(serusbcfg1.usbp, &usbcfg);
  usbConnectBus(serusbcfg1.usbp);

  /* Initializing the virtual timer. */
  chVTObjectInit(&vt);
  /* Setting the button line as digital input without pull resistors. */
  palSetLineMode(EXTRST, PAL_MODE_INPUT);
  /* Enabling the event and associating the callback. */
  palEnableLineEvent(EXTRST, PAL_EVENT_MODE_FALLING_EDGE);
  palSetLineCallback(EXTRST, button_cb, NULL);

  mypwmInit();
  SPI_init();
  i2c_init();
  start_upload_thread();
  /*
   * Shell manager initialization.
   * Event zero is shell exit.
   */
  shellInit();
  chEvtRegister(&shell_terminated, &shell_el, 0);

  /*
   * Normal main() thread activity, in this demo it does nothing except
   * sleeping in a loop and check the button state.
   */
  while (true) {
#if USB_SHELL == 1
    if (SHELLPORT.config->usbp->state == USB_ACTIVE) {
      /* Starting shells.*/
      if (shelltp == NULL) {
        shelltp = chThdCreateFromHeap(NULL, SHELL_WA_SIZE,
                                       "shell1", NORMALPRIO + 1,
                                       shellThread, (void *)&shell_cfg1);
      }
#else
    if (!shelltp)
      shelltp = chThdCreateFromHeap(NULL, SHELL_WA_SIZE,
                                    "shell", NORMALPRIO + 1,
                                    shellThread, (void *)&shell_cfg1);
    else if (chThdTerminatedX(shelltp)) {
      chThdRelease(shelltp);    /* Recovers memory of the previous shell.   */
      shelltp = NULL;           /* Triggers spawning of a new shell.        */
    }
#endif
    /* Waiting for an exit event then freeing terminated shells.*/
    chEvtWaitAny(EVENT_MASK(0));
    if (chThdTerminatedX(shelltp)) {
      chThdRelease(shelltp);
      shelltp = NULL;
    }
  }
  chThdSleepMilliseconds(1000);
//  palTogglePad(GPIOA, 5);
}

