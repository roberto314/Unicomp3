/*
 * fs_helper.h
 *
 *  Created on: Sep 26, 2023
 *      Author: rob
 */

#include "ch.h"
#include "hal.h"
#include "ff.h"
#include "chprintf.h"
#include "stdlib.h"
#include "string.h" /* for memset */
#include "shell.h"

#ifndef USERLIB_INCLUDE_FS_HELPER_H_
#define USERLIB_INCLUDE_FS_HELPER_H_

void verbose_error(BaseSequentialStream *chp, FRESULT err);
void cmd_tree(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_create(BaseSequentialStream *chp, int argc, char *argv[]);
//void cat(BaseSequentialStream *chp, char * fname);
void cmd_cat(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_exec(BaseSequentialStream *chp, int argc, char *argv[]);
uint8_t exec(ShellConfig* scfg);
void cmd_cd(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_mkdir(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_rmdir(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_mount(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_umount(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_setlabel(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_getlabel(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_dir(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_pwd(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_rm(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_mv(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_free(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_copy(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_kar(BaseSequentialStream *chp, int argc, char *argv[]);

#endif /* USERLIB_INCLUDE_FS_HELPER_H_ */
