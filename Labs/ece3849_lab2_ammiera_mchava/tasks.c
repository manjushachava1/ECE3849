/*
 * tasks.c
 *
 *  Created on: Nov 19, 2021
 *      Author: alexm
 */

//// INCLUDES & DEFINES ////
#include "sampling.h"
#include "settings.h"

/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <xdc/cfg/global.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>


//// EXPORTED VARIABLES ////
extern uint16_t displayADCBuffer[ADC_BUFFER_SIZE];

//// GLOBAL VARIABLES ////
float gVoltageScale;


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
void waveform_task(UArg arg0, UArg arg1, tContext sContext)
{
    IntMasterEnable();
    Semaphore_pend(semTask0, BIOS_WAIT_FOREVER); // blocks until signaled

    int triggerIndex;

    triggerIndex = RisingTrigger(); // finds trigger index
    CopySignal(sContext, triggerIndex); // copies signal starting from and ending 1/2 way behind the trigger index

    Semaphore_post(semTask2); // signals processing task
}

void processing_task(tContext sContext){
    IntMasterEnable();

    Semaphore_pend(semTask2, BIOS_WAIT_FOREVER); // blocks until signaled

    // scales captured waveform
    gVoltageScale = GetVoltageScale();
    ADCSampleScaling(sContext, gVoltageScale);

    // signal Display Task to draw waveform
    Semaphore_post(semTask1); // signals display task
}

void display_task(tContext sContext){
    IntMasterEnable();

    Semaphore_pend(semTask1, BIOS_WAIT_FOREVER);

    // drawing grid, scales, and waveform
    DrawGrid(sContext);
    WriteVoltageScale(sContext, gVoltageScale);
    WriteTimeScale(2, sContext);
    DrawFrame(sContext);

    Semaphore_post(semTask0); // signals waveform task
}


