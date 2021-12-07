/*
 * ECE 3849 Lab2 starter project
 *
 * Gene Bogdanov    9/13/2017
 */

 //// INCLUDES & DEFINES ////

// XDCtools Header files
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <xdc/cfg/global.h>

// BIOS header files
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

#include <stdint.h>
#include <stdbool.h>
#include "driverlib/interrupt.h"
#include "sampling.h"
#include "driverlib/fpu.h"
#include "driverlib/sysctl.h"
#include "Crystalfontz128x128_ST7735.h"

// Code header files
#include "sampling.h"
#include "string.h"
#include "fifo.h"
#include "settings.h"
#include "buttons.h"
#include <math.h>




//// GLOBAL VARIABLES ////
volatile uint32_t gTime; // time in hundredth of a second
uint32_t gSystemClock = 120000000; // [Hz] system clock frequency
tContext sContext;




//// MAIN FUNCTION ////

// METHOD CALL: main.c
// DESCRIPTION: initializes system
// INPUTS: void
// OUTPUTS: void
// AUTHOR: professor
// REVISION HISTORY: NA
// NOTES:
//      - Does ADCInit() go before DMA_Init()?
// TODO: NA
int main(void)
{
    IntMasterDisable();

    // Initialize the system clock to 120 MHz
    gSystemClock = SysCtlClockFreqSet(
            SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL
                    | SYSCTL_CFG_VCO_480,
            120000000);

    Crystalfontz128x128_Init(); // Initialize the LCD display driver
    Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP); // set screen orientation

    // hardware initialization
    SignalInit(); // initialize signal generator hardware
    ADCInit(); // initialize ADC
    ButtonInit(); // initialize board buttons
    DMA_Init();

    GrContextInit(&sContext, &g_sCrystalfontz128x128); // Initialize the grlib graphics context
    GrContextFontSet(&sContext, &g_sFontFixed6x8); // select font

    /* Start BIOS */
    BIOS_start();

    return (0);
}

void task0_func(UArg arg1, UArg arg2)
{
    IntMasterEnable();

    while (true)
    {
        // do nothing
    }
}
