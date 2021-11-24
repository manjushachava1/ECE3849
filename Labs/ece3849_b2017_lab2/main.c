/*
 * ECE 3849 Lab2 prototype
 *
 * Gene Bogdanov    9/13/2017
 */
/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/fpu.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "driverlib/systick.h"
#include "Crystalfontz128x128_ST7735.h"
#include <stdio.h>
#include "buttons.h"
#include "sampling.h"
#include "oscilloscope.h"

uint32_t gSystemClock = 120000000; // [Hz] system clock frequency
tContext sContext;  // grlib context

/*
 *  ======== main ========
 */
int main(void)
{
    IntMasterDisable();

    // debugging: setup SysTick timer for the purpose of profiling
    SysTickPeriodSet(1 << 24); // full 24-bit counter
    SysTickEnable();

    Crystalfontz128x128_Init(); // Initialize the LCD display driver
    Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP); // set screen orientation

    GrContextInit(&sContext, &g_sCrystalfontz128x128); // Initialize the grlib graphics context

    ADCInit();      // initialize the peripherals that sample the waveform into a buffer
    ButtonInit();   // initialize the peripherals that scan the user buttons

    /* Start BIOS */
    BIOS_start();

    return (0);
}
