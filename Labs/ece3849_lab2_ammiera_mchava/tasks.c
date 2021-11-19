/*
 * tasks.c
 *
 *  Created on: Nov 19, 2021
 *      Author: alexm
 */

//// INCLUDES & DEFINES ////
#include "sampling.h"



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
    Semaphore_pend(semaTask0, BIOS_WAIT_FOREVER); // blocks until signalled

    int triggerIndex;

    triggerIndex = RisingTrigger(); // finds trigger index
    CopySignal(sContext, triggerIndex); // copies signal starting from and ending 1/2 way behind the trigger index

    Semaphore_post(semTask2); // signals processing task
}


