/*
 * sdcard.h
 *
 *  Created on: Nov 5, 2024
 *      Author: rob
 */

#ifndef USERLIB_INCLUDE_SDCARD_H_
#define USERLIB_INCLUDE_SDCARD_H_

void InsertHandler(eventid_t id);
void RemoveHandler(eventid_t id);
void sdcard_init(void);

#endif /* USERLIB_INCLUDE_SDCARD_H_ */
