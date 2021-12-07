/*
 * fifo.c
 *
 *  Created on: Nov 9, 2021
 *      Author: alexm
 */

#include "fifo.h"

//// GLOBALS ////
DataType fifo[BUTTON_FIFO_SIZE]; // button FIFO storage array
volatile int fifo_head = 0; // index of the first item in the FIFO
volatile int fifo_tail = 0; // index of one step past the last item




//// FUNCTIONS ////
// put data into the FIFO, skip if full
// returns 1 on success, 0 if FIFO was full
int fifo_put(DataType data) {
    int new_tail = fifo_tail + 1;

    if (new_tail >= BUTTON_FIFO_SIZE) {
        new_tail = 0; // warp around
    }

    if (fifo_head != new_tail) { // if the FIFO is not full
        fifo[fifo_tail] = data; // store data into the FIFO
        fifo_tail = new_tail;
        return 1; // data was successfully placed in FIFO
    }

    return 0; // data could not be placed in the FIFO because it was full
}

// get data from FIFO
// returns 1 on success, 0 if FIFO was empty
int fifo_get(DataType *data) {
    if (fifo_head != fifo_tail) { // if the FIFO is not empty
        *data = fifo[fifo_head]; // read data from FIFO

        // updates fifo_head
        // instead of using fifo_head++ use the below if statement to avoid buffer overflow
        if (fifo_head + 1 >= BUTTON_FIFO_SIZE) {
            fifo_head = 0; // wrap around
        } else {
            fifo_head++;
        }

        if (fifo_head >= BUTTON_FIFO_SIZE) {
            return 1; // data was successfully read from the FIFO
        }
    }

    return 0; // the FIFO was empty
}


