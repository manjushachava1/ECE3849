/*
 * oscilloscope.c
 *
 *  Created on: Aug 12, 2012
 *      Author: Gene Bogdanov
 */

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Mailbox.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <xdc/runtime/Timestamp.h>
#include <xdc/cfg/global.h>

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "driverlib/adc.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "oscilloscope.h"
#include "sampling.h"
#include "buttons.h"
#include "settings.h"
#include "spectrum.h"

// time scale display strings
static const char *const gTimeScaleStr[TIME_SCALE_STEPS] =
        { " 20 us", " 50 us", "100 us", "200 us", "500 us", "  1 ms", "  2 ms",
          "  5 ms", " 10 ms", " 20 ms", " 50 ms", "100 ms" };

// frequency scale display strings
static const char *const gFreqScaleStr[TIME_SCALE_STEPS] =
        { "20 kHz", "7.8kHz", "3.9kHz", " 2 kHz", "781 Hz", "391 Hz", "195 Hz",
          "78.1Hz", "39.1Hz", "19.5Hz", "7.81Hz", "3.91Hz" };

// sampling rates for different time scales
const float gSamplingRate[TIME_SCALE_STEPS] = { 1000000, PIXELS_PER_DIV / 50e-6,
PIXELS_PER_DIV / 100e-6,
                                                PIXELS_PER_DIV / 200e-6,
                                                PIXELS_PER_DIV / 500e-6,
                                                PIXELS_PER_DIV / 1e-3,
                                                PIXELS_PER_DIV / 2e-3,
                                                PIXELS_PER_DIV / 5e-3,
                                                PIXELS_PER_DIV / 10e-3,
                                                PIXELS_PER_DIV / 20e-3,
                                                PIXELS_PER_DIV / 50e-3,
                                                PIXELS_PER_DIV / 100e-3 };

// voltage scale display strings
static const char *const gVoltageScaleStr[VOLTAGE_SCALE_STEPS] = { "100 mV",
                                                                   "200 mV",
                                                                   "500 mV",
                                                                   "  1 V" };

// volts/div for different voltage scales
static const float gVoltsPerDiv[VOLTAGE_SCALE_STEPS] = { 0.1, 0.2, 0.5, 1.0 };

static int16_t gWaveformBuffer[NFFT];	        // waveform buffer
static int16_t gProcessedWaveform[FRAME_SIZE_X];// processed waveform for display
int gTimeSetting = 0;							// time scale index
static int gVoltageSetting = 2;		            // voltage scale index
static int gTriggerY = FRAME_SIZE_Y / 2;// trigger level y-coordinate on the LCD
static bool gTriggerRising = true;			    // trigger slope spec
static int32_t gTriggerLevel = ADC_OFFSET;	    // trigger level in ADC units
static int gAdjusting = 2; // which setting is currently being adjusted (0 = time, 1 = voltage, 2 = trigger)
static float gScale;		// floating point ADC to pixels conversion factor
extern bool gSpectrumMode = false; // true if FFT mode, false if time domain mode

extern uint32_t gSystemClock;
extern tContext sContext;  // grlib context

// benchmarking variables
uint32_t gTriggerSearchTime = 0;

void OscilloscopeTimeScale(void)
{
    if (gTimeSetting == 0)
    {
        // special setting that runs the ADC at full speed
        ADCSequenceDisable(ADC1_BASE, 0);
        ADCSequenceConfigure(ADC1_BASE, 0, ADC_TRIGGER_ALWAYS, 0);
        ADCSequenceEnable(ADC1_BASE, 0);
    }
    else
    {
        // change the timer period
        TimerDisable(TIMER1_BASE, TIMER_A);
        TimerLoadSet(TIMER1_BASE, TIMER_A,
                     (float) gSystemClock / gSamplingRate[gTimeSetting] - 0.5f);
        TimerEnable(TIMER1_BASE, TIMER_A);
        // trigger the ADC on the timer
        ADCSequenceDisable(ADC1_BASE, 0);
        ADCSequenceConfigure(ADC1_BASE, 0, ADC_TRIGGER_TIMER, 0);
        ADCSequenceEnable(ADC1_BASE, 0);
    }
}

void OscilloscopeVoltageScale(void)
{
    // floating point ADC to pixels conversion factor
    gScale = (ADC_RANGE * PIXELS_PER_DIV)
            / ((1 << ADC_BITS) * gVoltsPerDiv[gVoltageSetting]);
}

void OscilloscopeTriggerLevel(void)
{
    // trigger level in ADC units
    gTriggerLevel = (long) ((FRAME_SIZE_Y / 2 - gTriggerY) / gScale + ADC_OFFSET
            + 0.5);
}

void OscilloscopeUserInput(char button)
{
    // process user button presses
    switch (button)
    {
    case 'U': // up
        switch (gAdjusting)
        {
        case 0:
            if (++gTimeSetting >= TIME_SCALE_STEPS)
                gTimeSetting = TIME_SCALE_STEPS - 1;
            OscilloscopeTimeScale();
            break;
        case 1:
            if (++gVoltageSetting >= VOLTAGE_SCALE_STEPS)
                gVoltageSetting = VOLTAGE_SCALE_STEPS - 1;
            OscilloscopeVoltageScale();
            OscilloscopeTriggerLevel();
            break;
        case 2:
            if (--gTriggerY < 0)
                gTriggerY = 0;
            OscilloscopeTriggerLevel();
            break;
        }
        break;
    case 'D': // down
        switch (gAdjusting)
        {
        case 0:
            if (--gTimeSetting < 0)
                gTimeSetting = 0;
            OscilloscopeTimeScale();
            break;
        case 1:
            if (--gVoltageSetting < 0)
                gVoltageSetting = 0;
            OscilloscopeVoltageScale();
            OscilloscopeTriggerLevel();
            break;
        case 2:
            if (++gTriggerY >= FRAME_SIZE_Y)
                gTriggerY = FRAME_SIZE_Y - 1;
            OscilloscopeTriggerLevel();
            break;
        }
        break;
    case 'L': // left
        if (--gAdjusting < 0)
            gAdjusting = 2;
        break;
    case 'R': // right
        if (++gAdjusting > 2)
            gAdjusting = 0;
        break;
    case 'S': // select
        gTriggerRising = !gTriggerRising; // toggle trigger slope
        break;
    case 'A': // BoosterPack button 1
        gSpectrumMode = !gSpectrumMode; // switch between spectrum and oscilloscope mode
        break;
    }
}

void OscilloscopeDrawGrid(tContext *psContext)
{
    int i;
    // draw grid
    GrContextForegroundSet(psContext, ClrDarkBlue); // grid line color
    for (i = (FRAME_SIZE_Y / 2) % PIXELS_PER_DIV; i < FRAME_SIZE_Y; i +=
    PIXELS_PER_DIV)
        GrLineDrawH(psContext, 0, FRAME_SIZE_X - 1, i);
    for (i = (FRAME_SIZE_X / 2) % PIXELS_PER_DIV; i < FRAME_SIZE_X; i +=
    PIXELS_PER_DIV)
    {
        if (i == FRAME_SIZE_X / 2)
            GrContextForegroundSet(psContext, ClrBlue); // center axis in lighter color
        else
            GrContextForegroundSet(psContext, ClrDarkBlue);
        GrLineDrawV(psContext, i, 0, FRAME_SIZE_Y - 1);
    }
}

void OscilloscopeSpectrumDrawGrid(tContext *psContext)
{
    int i;
    // draw grid
    GrContextForegroundSet(psContext, ClrDarkBlue); // grid line color
    for (i = 0; i < FRAME_SIZE_X; i += PIXELS_PER_DIV)
        GrLineDrawV(psContext, i, 0, FRAME_SIZE_Y - 1);
    for (i = 0; i < FRAME_SIZE_Y; i += PIXELS_PER_DIV)
    {
        if (i == PIXELS_PER_DIV)
            GrContextForegroundSet(psContext, ClrBlue); // 0 dBV axis in lighter color
        else
            GrContextForegroundSet(psContext, ClrDarkBlue);
        GrLineDrawH(psContext, 0, FRAME_SIZE_X - 1, i);
    }
}

void OscilloscopeDrawSettings(tContext *psContext)
{
    // highlight the setting selected for adjustment
    tRectangle rect =
            { gAdjusting * 8 * 6, 0, gAdjusting * 8 * 6 + 6 * 6 - 1, 7 };
    GrContextForegroundSet(psContext, ClrDarkGreen);
    GrRectFill(psContext, &rect);
    // trigger level line
    GrContextForegroundSet(psContext, ClrBlue);
    GrLineDrawH(psContext, 0, FRAME_SIZE_X - 1, gTriggerY);
    // time scale string
    GrContextForegroundSet(psContext, ClrWhite);
    GrStringDraw(psContext, gTimeScaleStr[gTimeSetting], /*length*/-1, /*x*/0, /*y*/
                 0, /*opaque*/false);
    // voltage scale string
    GrStringDraw(psContext, gVoltageScaleStr[gVoltageSetting], /*length*/-1, /*x*/
                 8 * 6, /*y*/0, /*opaque*/false);
    // graphical trigger slope indicator
    if (gTriggerRising)
    {
        GrLineDrawH(psContext, TRIGGER_X, TRIGGER_X + 7, 7);
        GrLineDrawV(psContext, TRIGGER_X + 7, 7, 0);
        GrLineDrawH(psContext, TRIGGER_X + 7, TRIGGER_X + 14, 0);
        GrLineDraw(psContext, TRIGGER_X + 7, 2, TRIGGER_X + 5, 4);
        GrLineDraw(psContext, TRIGGER_X + 7, 2, TRIGGER_X + 9, 4);
    }
    else
    {
        GrLineDrawH(psContext, TRIGGER_X, TRIGGER_X + 7, 0);
        GrLineDrawV(psContext, TRIGGER_X + 7, 7, 0);
        GrLineDrawH(psContext, TRIGGER_X + 7, TRIGGER_X + 14, 7);
        GrLineDraw(psContext, TRIGGER_X + 7, 5, TRIGGER_X + 5, 3);
        GrLineDraw(psContext, TRIGGER_X + 7, 5, TRIGGER_X + 9, 3);
    }
}

void OscilloscopeSpectrumDrawSettings(tContext *psContext)
{
    // highlight the setting selected for adjustment
    tRectangle rect =
            { gAdjusting * 8 * 6, 0, gAdjusting * 8 * 6 + 6 * 6 - 1, 7 };
    GrContextForegroundSet(psContext, ClrDarkGreen);
    GrRectFill(psContext, &rect);
    GrContextForegroundSet(psContext, ClrWhite);
    // frequency scale string
    GrStringDraw(psContext, gFreqScaleStr[gTimeSetting], /*length*/-1, /*x*/0, /*y*/
                 0, /*opaque*/false);
    // voltage scale string
    GrStringDraw(psContext, "20 dBV", /*length*/-1, /*x*/8 * 6, /*y*/0, /*opaque*/
                 false);
}

int OscilloscopeTrigger(void)	// search for the trigger
{
    int x_initial = gADCBufferIndex - FRAME_SIZE_X / 2;
    int x = x_initial;
    int i, y, y_last;

    y_last = gADCBuffer[ADC_BUFFER_WRAP(x)];
    if (gTriggerRising)
    {
        for (i = 0; i < ADC_BUFFER_SIZE / 2; i++, y_last = y)
        {
            y = gADCBuffer[ADC_BUFFER_WRAP(--x)]; // step back and read value
            if (y <= gTriggerLevel && y_last > gTriggerLevel)
                break;
        }
    }
    else
    { // falling slope
        for (i = 0; i < ADC_BUFFER_SIZE / 2; i++, y_last = y)
        {
            y = gADCBuffer[ADC_BUFFER_WRAP(--x)]; // step back and read value
            if (y >= gTriggerLevel && y_last < gTriggerLevel)
                break;
        }
    }
    if (i >= ADC_BUFFER_SIZE / 2) // trigger was not found
        x = x_initial;
    return x;
}

void OscilloscopeCopyWaveform(int x, int n)
// x = starting index in the ADC buffer
// n = number of samples to copy
{
    int i;
    // copy a screen-full of samples to the waveform buffer
    for (i = 0; i < n; i++)
    {
        gWaveformBuffer[i] = (int16_t) gADCBuffer[ADC_BUFFER_WRAP(x + i)];
    }
}

void OscilloscopeDrawWaveform(tContext *psContext)
{
    int i, y, y_last;
    // draw waveform
    y_last = gProcessedWaveform[0];
    for (i = 1; i < FRAME_SIZE_X; i++, y_last = y)
    {
        y = gProcessedWaveform[i];
        GrLineDraw(psContext, i - 1, y_last, i, y);
    }
}

//void waveform_task(UArg arg0, UArg arg1)
//{
//    int trigger_index;
//
//    OscilloscopeVoltageScale(); // init oscilloscope settings
//    OscilloscopeTimeScale();
//    OscilloscopeTriggerLevel();
//
//    IntMasterEnable();
//
//    while (true)
//    {
//        Semaphore_pend(WaveformSem, BIOS_WAIT_FOREVER);
//
//        if (!gSpectrumMode)
//        { // time domain mode
//            uint32_t trigger_start = Timestamp_get32();
//            trigger_index = OscilloscopeTrigger();
//            OscilloscopeCopyWaveform(trigger_index - FRAME_SIZE_X / 2,
//            FRAME_SIZE_X); // copy triggered waveform to a local buffer
//            uint32_t trigger_search_time = Timestamp_get32() - trigger_start;
//            if (trigger_search_time > gTriggerSearchTime)
//                gTriggerSearchTime = trigger_search_time;
//        }
//        else
//        { // spectrum mode
//            OscilloscopeCopyWaveform(gADCBufferIndex - NFFT + 1, NFFT);
//        }
//
//        Semaphore_post(ProcessingSem);  // signal Waveform Processing task
//    }
//}

