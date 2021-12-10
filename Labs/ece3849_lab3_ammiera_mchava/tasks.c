/*
 * tasks.c
 *
 *  Created on: Nov 19, 2021
 *      Author: alexm
 */

//// INCLUDES & DEFINES ////
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "driverlib/adc.h"
#include "buttons.h"
#include "sampling.h"
#include "settings.h"
#include "oscilloscope.h"
#include "spectrum.h"

// XDCtools Header files
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <xdc/cfg/global.h>

// BIOS Header files
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

//// GLOBAL VARIABLES ////
float gVoltageScale;

/* imported variables */
extern uint16_t displayADCBuffer[ADC_BUFFER_SIZE];
extern tContext sContext;

// benchmarking variables
uint32_t gButtonLatency = 0;
uint32_t gButtonResponseTime = 0;
uint32_t gButtonMissedDeadlines = 0;

int bIsSpecSampled = 0; // false

//// FUNCTIONS ////

// METHOD CALL: RTOS?
// DESCRIPTION:
//      1. Searches for trigger in ADC buffer
//      2. Copies the trigger's waveform into the waveform buffer
//      3. Signals the Processing Task
//      4. Blocks
// INPUTS:
//      * arg1 - ?
//      * arg2 - ?
// OUTPUTS: void
// AUTHOR: ammiera
// REVISION HISTORY: 11/19/2021
// NOTES: NA
// TODO:
//      * Need to figure out what is calling this task
//      * Need to figure out what arg0 and arg1 is for
//      * Testing

void user_input_task(UArg arg1, UArg arg2)
{
    char button;

    while (true)
    {
        Mailbox_pend(ButtonMailbox, &button, BIOS_WAIT_FOREVER);

        OscilloscopeUserInput(button);

        Semaphore_post(DisplaySem);  // request update of the display
    }
}

void waveform_task(UArg arg1, UArg arg2)
{
    IntMasterEnable();
    int32_t triggerIndex;
    int32_t bufferIndex;

    while (1)
    {

        Semaphore_pend(WaveformSem, BIOS_WAIT_FOREVER); // blocks until signaled

        Semaphore_pend(CSSem, BIOS_WAIT_FOREVER);
        if (gSpectrumMode) // spectrum mode
        {
            if (!bIsSpecSampled)
            {
                get_spec_samples();
            }
        }
        else
        {
            bufferIndex = getADCBufferIndex();
            triggerIndex = RisingTrigger(bufferIndex); // finds trigger index
            CopySignal(triggerIndex); // copies signal starting from and ending 1/2 way behind the trigger index
            gVoltageScale = GetVoltageScale();
            ADCSampleScaling(gVoltageScale);
        }
        Semaphore_post(CSSem);

        Semaphore_post(ProcessingSem); // signals processing task
    }
}

void display_task(UArg arg1, UArg arg2)
{
    tRectangle rectFullScreen = { 0, 0, GrContextDpyWidthGet(&sContext) - 1,
    GrContextDpyHeightGet(&sContext) - 1 };
    GrContextFontSet(&sContext, &g_sFontFixed6x8); // select font

    while (true)
    {
        Semaphore_pend(DisplaySem, BIOS_WAIT_FOREVER);

        GrContextForegroundSet(&sContext, ClrBlack);
        GrRectFill(&sContext, &rectFullScreen); // fill screen with black

        if (gSpectrumMode)
        {
//             draw everything to the local frame buffer
           display_spec_grid();
           display_frequency_scale();
           display_dB_scale();
           display_spec_waveform();
        }
        else
        {
            // draw everything to the local frame buffer
            DrawGrid();
            DrawTriggerSlope();
            WriteTimeScale(2);
            WriteVoltageScale(gVoltageScale);
            ADCSampleScaling(gVoltageScale);
            // WriteCPULoad(1);
        }
    }
}

void processing_task(UArg arg1, UArg arg2)
{
    while (true)
    {
        Semaphore_pend(ProcessingSem, BIOS_WAIT_FOREVER);

        if (gSpectrumMode && !bIsSpecSampled) {
            window_time_dom();
            compute_FFT();
            convert_to_dB();
            bIsSpecSampled = 1;

        }

        Semaphore_post(DisplaySem); // request update of the display
        Semaphore_post(WaveformSem); // request another waveform acquisition
    }
}

void button_task(UArg arg1, UArg arg2)
{
    char msg;
    uint32_t timer0_period = TimerLoadGet(TIMER0_BASE, TIMER_A) + 1;

    while (true)
    {
        Semaphore_pend(ButtonSem, BIOS_WAIT_FOREVER);

        uint32_t t = timer0_period - TimerValueGet(TIMER0_BASE, TIMER_A);
        if (t > gButtonLatency)
            gButtonLatency = t; // measure latency

        uint32_t gpio_buttons = (~GPIOPinRead(GPIO_PORTJ_BASE, 0xff)
                & (GPIO_PIN_1 | GPIO_PIN_0)) // EK-TM4C1294XL buttons in positions 0 and 1
        | (~GPIOPinRead(GPIO_PORTH_BASE, 0xff) & GPIO_PIN_1) << 1 // BoosterPack button 1 in position 2
        | (~GPIOPinRead(GPIO_PORTK_BASE, 0xff) & GPIO_PIN_6) >> 3 // BoosterPack button 2 in position 3
        | (~GPIOPinRead(GPIO_PORTD_BASE, 0xff) & GPIO_PIN_4); // BoosterPack Joystick Select button in position 4

        uint32_t old_buttons = gButtons;    // save previous button state
        ButtonDebounce(gpio_buttons); // Run the button debouncer. The result is in gButtons.
        ButtonReadJoystick(); // Convert joystick state to button presses. The result is in gButtons.
        uint32_t presses = ~old_buttons & gButtons; // detect button presses (transitions from not pressed to pressed)
        presses |= ButtonAutoRepeat(); // autorepeat presses if a button is held long enough

        if (presses & 1)
        {        // EK-TM4C1294XL button 1 pressed
            msg = '1';
            Mailbox_post(ButtonMailbox, &msg, BIOS_WAIT_FOREVER);
        }
        if (presses & (1 << 1))
        { // EK-TM4C1294XL button 2 pressed
            msg = '2';
            Mailbox_post(ButtonMailbox, &msg, BIOS_WAIT_FOREVER);
        }
        if (presses & (1 << 2))
        { // BoosterPack button 1 pressed
            msg = 'A';
            Mailbox_post(ButtonMailbox, &msg, BIOS_WAIT_FOREVER);
        }
        if (presses & (1 << 3))
        { // BoosterPack button 2 pressed
            msg = 'B';
            Mailbox_post(ButtonMailbox, &msg, BIOS_WAIT_FOREVER);
        }
        if (presses & (1 << 4))
        { // joystick select button pressed
            msg = 'S';
            Mailbox_post(ButtonMailbox, &msg, BIOS_WAIT_FOREVER);
        }
        if (presses & (1 << 5))
        { // joystick deflected right
            msg = 'R';
            Mailbox_post(ButtonMailbox, &msg, BIOS_WAIT_FOREVER);
        }
        if (presses & (1 << 6))
        { // joystick deflected left
            msg = 'L';
            Mailbox_post(ButtonMailbox, &msg, BIOS_WAIT_FOREVER);
        }
        if (presses & (1 << 7))
        { // joystick deflected up
            msg = 'U';
            Mailbox_post(ButtonMailbox, &msg, BIOS_WAIT_FOREVER);
        }
        if (presses & (1 << 8))
        { // joystick deflected down
            msg = 'D';
            Mailbox_post(ButtonMailbox, &msg, BIOS_WAIT_FOREVER);
        }

        if (Semaphore_getCount(ButtonSem))
        { // next event occurred
            gButtonMissedDeadlines++;
            t = 2 * timer0_period; // timer overflowed since last event
        }
        else
            t = timer0_period;
        t -= TimerValueGet(TIMER0_BASE, TIMER_A);
        if (t > gButtonResponseTime)
            gButtonResponseTime = t; // measure response time
    }
}

void button_clock(UArg arg)
{
    Semaphore_post(ButtonSem);
}
