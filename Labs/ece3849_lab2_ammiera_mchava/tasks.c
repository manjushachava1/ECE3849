/*
 * tasks.c
 *
 *  Created on: Nov 19, 2021
 *      Author: alexm
 */

//// INCLUDES & DEFINES ////




//// GLOBAL VARIABLES ////




//// FUNCTIONS ////

// METHOD CALL: RTOS?
// DESCRIPTION:
//      1. Searches for trigger in ADC buffer
//      2. Copies the trigger's waveform into the waveform buffer
//      3. Signals the Processing Task
//      4. Blocks
// INPUTS:
//      * arg0 - ?
//      * arg1 - ?
// OUTPUTS: void
// AUTHOR: ammiera
// REVISION HISTORY: 11/19/2021
// NOTES: NA
// TODO:
//      * Need to figure out what is calling this task
//      * Need to figure out what arg0 and arg1 is for
//      * Testing
void task0_waveform(UArg arg0, UArg arg1)
{
    unsigned long t;

    while(1) {
        Semaphore_pend(semaphore0, BIOS_WAIT_FOREVER);

        t = TIMER0_PERIOD - TIMER0_TAR_R;
        if (t > event0_latency) event0_latency = t; // measure latency

        delay_us(EVENT0_EXECUTION_TIME); // handle event0
        event0_count++;

        if (Semaphore_getCount(semaphore0)) { // next event occurred
            event0_missed_deadlines++;
            t = 2 * TIMER0_PERIOD; // timer overflowed since last event
        }
        else t = TIMER0_PERIOD;
        t -= TIMER0_TAR_R;
        if (t > event0_response_time) event0_response_time = t; // measure response time
    }
}


