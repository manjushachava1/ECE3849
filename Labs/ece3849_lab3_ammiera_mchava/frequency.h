/*
 * frequency.h
 *
 *  Created on: Dec 7, 2021
 *      Author: root
 */

#ifndef FREQUENCY_H_
#define FREQUENCY_H_

//// INCLUDES & DEFINES ////
#include <xdc/std.h>

void TimerInit(void);
void TimerCapture_ISR(UArg arg1);
void timercapture_ISR(void);


#endif /* FREQUENCY_H_ */
