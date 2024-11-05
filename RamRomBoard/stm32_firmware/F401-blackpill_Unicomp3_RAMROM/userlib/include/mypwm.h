/*
 * pwm.h
 *
 *  Created on: Sep 23, 2020
 *      Author: rob
 */

#ifndef USERLIB_INCLUDE_MYPWM_H_
#define USERLIB_INCLUDE_MYPWM_H_

#define CLK1          PAL_LINE(GPIOB, 4U)  // 

extern void mypwmInit(void);
extern void cmd_change_freq(BaseSequentialStream *chp, int argc, char *argv[]);
extern void cmd_change_pw4(BaseSequentialStream *chp, int argc, char *argv[]);


#endif /* USERLIB_INCLUDE_MYPWM_H_ */
