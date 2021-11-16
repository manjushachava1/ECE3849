/*
 * fifo.h
 *
 *  Created on: Nov 9, 2021
 *      Author: alexm
 */

#ifndef FIFO_H_
#define FIFO_H_

//// INCLUDES & DEFINES ////
#define BUTTON_FIFO_SIZE 11 // FIFO capacity is 1 item fewer
typedef char DataType;




//// FUNCTION PROTOTYPES ////
int fifo_put(DataType data);
int fifo_get(DataType *data);

#endif /* FIFO_H_ */
