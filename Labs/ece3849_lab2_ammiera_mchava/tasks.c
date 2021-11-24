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
void waveform_task(UArg arg1, UArg arg2)
{
    IntMasterEnable();

    while (1)
    {

        Semaphore_pend(WaveformSem, BIOS_WAIT_FOREVER); // blocks until signaled

        int triggerIndex;

        Semaphore_pend(CSSem, BIOS_WAIT_FOREVER);
        triggerIndex = RisingTrigger(); // finds trigger index
        CopySignal(triggerIndex); // copies signal starting from and ending 1/2 way behind the trigger index
        Semaphore_post(CSSem);

        Semaphore_post(ProcessingSem); // signals processing task
    }
}

void processing_task(UArg arg1, UArg arg2)
{
//    IntMasterEnable();
    while (1)
    {
        Semaphore_pend(ProcessingSem, BIOS_WAIT_FOREVER); // blocks until signaled

        // scales captured waveform
        gVoltageScale = GetVoltageScale();
        ADCSampleScaling(gVoltageScale);

        // signal Display Task to draw waveform
        Semaphore_post(WaveformSem);
        Semaphore_post(DisplaySem); // signals display task
    }
}

void display_task(UArg arg1, UArg arg2)
{
//    IntMasterEnable();
    while (1)
    {
        Semaphore_pend(DisplaySem, BIOS_WAIT_FOREVER);

        // drawing grid, scales, and waveform
        DrawGrid();
        WriteVoltageScale(gVoltageScale);
        WriteTimeScale(2);
        DrawFrame();

        Semaphore_post(WaveformSem); // signals waveform task
    }
}

