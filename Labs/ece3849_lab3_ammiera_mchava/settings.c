/*
 * settings.c
 *
 *  Created on: Nov 15, 2021
 *      Author: alexm
 */

//// INCLUDES & DEFINES ////
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "inc/tm4c1294ncpdt.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "driverlib/adc.h"
#include "sampling.h"
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
#include "buttons.h"
#include "sampling.h"
#include "oscilloscope.h"


//// GLOBAL VARIABLES ////

// variables used to display scaled data
int16_t y[1024];
char str[50];

// imported variables
extern int bButtonPressed;
extern uint16_t stableADCBuffer[ADC_BUFFER_SIZE];
extern tContext sContext;

// indexing variables
uint16_t displayADCBuffer[ADC_BUFFER_SIZE];
int gVoltageScaleIdx = 0;
float gVoltageScale;
float const cVoltageScales[] = { 0.1, 0.2, 0.5, 1, 2 };
const char S1 = 'a';
const char S2 = 'b';
const char NA = 'c';


// time scale display strings
static const char * const gTimeScaleStr[TIME_SCALE_STEPS] =
        { " 20 us", " 50 us", "100 us", "200 us", "500 us", "  1 ms", "  2 ms",
          "  5 ms", " 10 ms", " 20 ms", " 50 ms", "100 ms" };


// CPU Variables
uint32_t count_unloaded = 0;
uint32_t count_loaded = 0;
float cpu_load = 0.0;
tContext sContext;


//// FUNCTIONS ////

// updates current voltage scale index based on FIFO button press and returns current voltage scale
float GetVoltageScale(void)
{
    char *pButtonPressed;
    char buttonPressed;
    int lVoltageScaleIdx = gVoltageScaleIdx; // local copy of voltage scale index
    float lVoltageScale;

    if (bButtonPressed)
    {
        fifo_get(pButtonPressed);
        buttonPressed = *pButtonPressed;
        bButtonPressed = 0;
    }
    else
    {
        buttonPressed = NA;
    }

    switch (buttonPressed)
    {
    case S1:
        if ((lVoltageScaleIdx + 1) >= NUM_VOLTAGE_SCALES)
        {
            lVoltageScaleIdx = 4;
        }
        else
        {
            lVoltageScaleIdx++;
        }
        break;

    case S2:
        if ((lVoltageScaleIdx - 1) <= 0)
        {
            lVoltageScaleIdx = 0;
        }
        else
        {
            lVoltageScaleIdx--;
        }
        break;

    default:
        // lVoltageScaleIdx = gVoltageScaleIdx
    }

    lVoltageScale = cVoltageScales[lVoltageScaleIdx];
    gVoltageScaleIdx = lVoltageScaleIdx;

    return lVoltageScale;
}

void DrawGrid(void)
{
    int32_t x = 4;
    int32_t y = 4;
    int i;
    GrContextForegroundSet(&sContext, ClrBlue); // blue lines

    // draws vertical grid
    GrLineDrawV(&sContext, x, BOTTOM_SCREEN, TOP_SCREEN);
    for (i = 1; i <= 6; i++)
    {
        GrLineDrawV(&sContext, (x + (20 * i)), BOTTOM_SCREEN, TOP_SCREEN);

        if (i == 3)
        {
            GrLineDrawV(&sContext, (x + (20 * i) + 1), BOTTOM_SCREEN,
            TOP_SCREEN);
            GrLineDrawV(&sContext, (x + (20 * i) - 1), BOTTOM_SCREEN,
            TOP_SCREEN);
        }
    }
    GrLineDrawV(&sContext, (x + (20 * i) + x), BOTTOM_SCREEN, TOP_SCREEN);

    // draws horizontal grid
    GrLineDrawH(&sContext, LEFTMOST_SCREEN, RIGHTMOST_SCREEN, y);
    for (i = 1; i <= 6; i++)
    {
        GrLineDrawH(&sContext, LEFTMOST_SCREEN, RIGHTMOST_SCREEN,
                    (y + (20 * i)));

        if (i == 3)
        {
            GrLineDrawH(&sContext, LEFTMOST_SCREEN, RIGHTMOST_SCREEN,
                        (y + (20 * i) + 1));
            GrLineDrawH(&sContext, LEFTMOST_SCREEN, RIGHTMOST_SCREEN,
                        (y + (20 * i) - 1));
        }
    }
    GrLineDrawH(&sContext, LEFTMOST_SCREEN, RIGHTMOST_SCREEN,
                (y + (20 * i) + y));

}

void WriteTimeScale(int timeScale)
{
    GrContextForegroundSet(&sContext, ClrWhite); // white text
//    GrStringDrawCentered(&sContext, timeScale + " us", 5, 20, 5, 1);
    GrStringDraw(&sContext, gTimeScaleStr[gTimeSetting], /*length*/-1, /*x*/0, /*y*/
                     0, /*opaque*/false);
}

void WriteVoltageScale(float voltageScale)
{
    // Print volts per division
    snprintf(str, sizeof(str), "%.1f V", voltageScale);
    GrContextForegroundSet(&sContext, ClrWhite);
    GrStringDraw(&sContext, str, /*length*/-1, /*x*/50, /*y*/3, /*opaque*/
                 false);
}

void ADCSampleScaling(float voltageScale)
{
    uint16_t sample;
    float fScale;
    int i;

    fScale = (VIN_RANGE * PIXELS_PER_DIV) / ((1 << ADC_BITS) * voltageScale);
    GrContextForegroundSet(&sContext, ClrYellow); // yellow text

    for (i = 0; i < LCD_HORIZONTAL_MAX; i++)
    {
        sample = stableADCBuffer[i];
        y[i] = ((int) (LCD_VERTICAL_MAX / 2))
                - (int) (fScale * ((int) sample - ADC_OFFSET));
        if (i != 0)
        {
            GrLineDraw(&sContext, i - 1, y[i - 1], i, y[i]);
        }
    }
}

/*
void DrawFrame(void)
{
    int i;
    for (i = 0; i < LCD_HORIZONTAL_MAX; i++)
    {
        if (i != 0)
        {
            GrLineDraw(&sContext, i - 1, displayADCBuffer[i - 1], i,
                       displayADCBuffer[i]);
        }
    }
}
*/

void DrawTriggerSlope(void)
{
    int y1 = 10;
    int y2 = y1 - 8; // 2
    int x1 = 104;
    int x2 = x1 + 8; // 112
    int x3 = x2 + 8; // 120

    int xTrig1 = x2 - 2; // 110
    int xTrig2 = x2 + 2; // 114
    int yTrig = ((y1 - y2) / 2) + 2; // 6

    GrContextForegroundSet(&sContext, ClrWhite); // white text

    // draws the step step function
    GrLineDrawH(&sContext, x1, x2, y1);
    GrLineDrawV(&sContext, x2, y1, y2);
    GrLineDrawH(&sContext, x2, x3, y2);

    // draws the trigger
    GrLineDrawH(&sContext, xTrig1, xTrig2, yTrig);
}

uint32_t WriteCPULoad(int flag)
{
    uint32_t i = 0;
    TimerIntClear(TIMER3_BASE, TIMER_TIMA_TIMEOUT);
    TimerEnable(TIMER3_BASE, TIMER_A); // start one-shot timer
    while (!(TimerIntStatus(TIMER3_BASE, false) & TIMER_TIMA_TIMEOUT))
    {
        i++;
    }
    if (flag == 1)
    {
        count_loaded = i;
        cpu_load = 1.0f - (float) count_loaded / count_unloaded; // compute CPU load
        snprintf(str, sizeof(str), "CPU Load = %.2f %%", cpu_load * (100));
        GrContextForegroundSet(&sContext, ClrWhite);
        GrStringDraw(&sContext, str, -1, 0, 118, false);
        GrFlush(&sContext); // flush the frame buffer to the LCD

    }
    return i;
}
