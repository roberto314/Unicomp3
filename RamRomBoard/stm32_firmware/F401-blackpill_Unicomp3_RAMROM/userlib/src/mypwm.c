/*
 * pwm.c
 *
 *  Created on: Sep 23, 2020
 *      Author: rob
 */


#include "ch.h"
#include "hal.h"

//#include "comm.h"
#include "chprintf.h"
#include "stdlib.h"
#include "string.h" /* for memset */
#include "shell.h"
#include "mypwm.h"

#define PERIOD 10000 // Period 1s (1Hz)

uint16_t dc=50; // in %
uint16_t halfper=PERIOD/2; // 2Hz



//static void pwmpcb(PWMDriver *pwmp) {
//
//  (void)pwmp;
//  //palClearPad(GPIOD, GPIOD_LED5);
//}
//
//static void pwmc1cb(PWMDriver *pwmp) {
//
//  (void)pwmp;
//  //palTogglePad(GPIOD, GPIOD_LED5);
//}


// for now only TIM3 on PB4 is used, but TIM2 on PB3 and TM1 on PB1 can also
// be used.
static PWMConfig pwmcfg = {
  10000U,                               /* 10kHz PWM clock halfperuency.   */
  PERIOD/2,                               /* Initial PWM period 0.5S.     */
  NULL,
  {
   {PWM_OUTPUT_ACTIVE_HIGH, NULL},
   {PWM_OUTPUT_DISABLED, NULL},
   {PWM_OUTPUT_DISABLED, NULL},
   {PWM_OUTPUT_DISABLED, NULL}
  },
  0,
  0
};

void cmd_change_freq(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argv;
  if (argc != 1) {
    chprintf(chp, "Changes Low Frequency TIC Output\r\n");
    chprintf(chp, "Period is: %u/10 ms\r\n", (uint16_t)(halfper));
    chprintf(chp, "Enter new Period in ms*10 (for 1kHz enter 10)\r\n");
    return;
  }
  halfper = atoi(argv[0]);
  pwmChangePeriod(&PWMD3, halfper);
  pwmEnableChannel(&PWMD3, 0, PWM_PERCENTAGE_TO_WIDTH(&PWMD3, dc*100));
}
void cmd_change_pw4(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argv;
  if (argc != 1) {
    chprintf(chp, "Changes Duty Cycle of Low Frequency TIC Output.\r\n");
    chprintf(chp, "Duty Cycle is: %u%%\r\n", dc);
    chprintf(chp, "Enter new Duty Cycle in %% (for 50%% enter 50)\r\n");
    chprintf(chp, "(for example: 25%% is 3/4 low.\r\n");
    return;
  }
  dc = atoi(argv[0]);
  chprintf(chp, "New duty Cycle is: %u or %u%%\r\n", PWM_PERCENTAGE_TO_WIDTH(&PWMD3, dc*100), dc);
  pwmEnableChannel(&PWMD3, 0, PWM_PERCENTAGE_TO_WIDTH(&PWMD3, dc*100));
}

void mypwmInit(void){
  pwmStart(&PWMD3, &pwmcfg);
  //pwmEnablePeriodicNotification(&PWMD1);
  palSetLineMode(CLK1, PAL_MODE_ALTERNATE(2) | PAL_STM32_OSPEED_HIGHEST);
  pwmEnableChannel(&PWMD3, 0,  PWM_PERCENTAGE_TO_WIDTH(&PWMD3, dc*100));
  //pwmEnableChannelNotification(&PWMD1, 0);
}
