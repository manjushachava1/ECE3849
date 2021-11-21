/*
 * ECE 3849 Lab2 starter project
 *
 * Gene Bogdanov    9/13/2017
 */
/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <xdc/cfg/global.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

#include <stdint.h>
#include <stdbool.h>
#include "driverlib/interrupt.h"
#include "sampling.h"


uint32_t gSystemClock = 120000000; // [Hz] system clock frequency

/*
 *  ======== main ========
 */
int main(void)
{
    IntMasterDisable();

    // hardware initialization goes here
    SignalInit(); // initialize signal generator hardware

    /* Start BIOS */
    BIOS_start();

    return (0);
}

void task0_func(UArg arg1, UArg arg2)
{
    IntMasterEnable();

    while (true) {
        // do nothing
    }
}

/**
 * main.c
 *
 * ECE 3849 Lab 1 Starter Project
 * Alex Miera, Manjusha Chava    11/10/2021
 *
 * This version is using the new hardware for B2021: the EK-TM4C1294XL LaunchPad with BOOSTXL-EDUMKII BoosterPack.
 *
 */

/*
//// INCLUDES & DEFINES ////
#include <stdint.h>
#include <stdbool.h>
#include "driverlib/fpu.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "Crystalfontz128x128_ST7735.h"
#include <stdio.h>
#include "buttons.h"
#include <math.h>
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/pwm.h"
#include "driverlib/pin_map.h"
#include "driverlib/timer.h"
#include "sampling.h"
#include "string.h"
#include "fifo.h"
#include "settings.h"




//// GLOBAL VARIABLES ////
uint32_t gSystemClock; // [Hz] system clock frequency
volatile uint32_t gTime; // time in hundredth of a second




//// MAIN FUNCTION ////
int main(void)
{
    IntMasterDisable(); // disables ISRS

    // Enable the Floating Point Unit, and permit ISRs to use it
    FPUEnable();
    FPULazyStackingEnable();

    // Initialize the system clock to 120 MHz
    gSystemClock = SysCtlClockFreqSet(
    SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480,
                                      120000000);

    Crystalfontz128x128_Init(); // Initialize the LCD display driver
    Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP); // set screen orientation

    SignalInit(); // initialize signal generator hardware

    tContext sContext;
    GrContextInit(&sContext, &g_sCrystalfontz128x128); // Initialize the grlib graphics context
    GrContextFontSet(&sContext, &g_sFontFixed6x8); // select font

    ADCInit();
    ButtonInit(); // initialize button hardware

    count_unloaded = WriteCPULoad(0, sContext);

    IntMasterEnable(); // enable ISRS

    tRectangle rectFullScreen = { 0, 0, GrContextDpyWidthGet(&sContext) - 1,
    GrContextDpyHeightGet(&sContext) - 1 }; // full-screen rectangle

    int triggerIndex;

    while (true)
    {
        GrContextForegroundSet(&sContext, ClrBlack);
        GrRectFill(&sContext, &rectFullScreen); // fill screen with black

        triggerIndex = RisingTrigger();
        CopySignal(sContext, triggerIndex);
        gVoltageScale = GetVoltageScale();

        // Display settings
        DrawGrid(sContext);
        DrawTriggerSlope(sContext);
        WriteTimeScale(2, sContext);
        WriteVoltageScale(sContext, gVoltageScale);
        ADCSampleScaling(sContext, gVoltageScale);
        WriteCPULoad(1, sContext);

        GrFlush(&sContext); // flush the frame buffer to the LCD
    }
}
*/
