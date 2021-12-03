/*
 * oscilloscope.h
 *
 *  Created on: Aug 12, 2012
 *      Author: Gene Bogdanov
 */

#ifndef OSCILLOSCOPE_H_
#define OSCILLOSCOPE_H_

#include "Crystalfontz128x128_ST7735.h"

//*****************************************************************************
// Oscilloscope functionality

#define FRAME_SIZE_X LCD_HORIZONTAL_MAX // screen dimensions
#define FRAME_SIZE_Y LCD_VERTICAL_MAX
#define PIXELS_PER_DIV 20		// LCD pixels per time or voltage division
#define TIME_SCALE_STEPS 12		// # of different time scales
#define VOLTAGE_SCALE_STEPS 4	// # of different voltage scales
#define ADC_RANGE 3.3			// ADC full scale range
#define ADC_OFFSET 2044			// input 0V in ADC units
#define SCALE_FRACTION_BITS 16	// number fixed point fraction bits to use when scaling samples
#define TRIGGER_X 105			// LCD x-coordinate of the trigger slope indicator

#define PI 3.14159265358979f
#define NFFT 1024               // FFT length

extern const float gSamplingRate[TIME_SCALE_STEPS];	// sampling rates for different time scales
extern int gTimeSetting;							// time scale index
extern bool gSpectrumMode;

// set the time scale as specified by gTimeSetting
void OscilloscopeTimeScale(void);

// set the voltage scale based on gVoltageSetting
void OscilloscopeVoltageScale(void);

// set the trigger level based on gTriggerY
void OscilloscopeTriggerLevel(void);

//// process the user button presses
void OscilloscopeUserInput(char button);

// draw the division grid to the frame buffer
void OscilloscopeDrawGrid(tContext *psContext);

// draw the time/div, volts/div and trigger slope indicators
void OscilloscopeDrawSettings(tContext *psContext);

// search for waveform trigger based on oscilloscope settings
// returns the ADC buffer position of the trigger, or -1 if not found
int OscilloscopeTrigger(void);

// copies samples from the ADC buffer to the waveform buffer
// x = starting position in the ADC buffer
// n = number of samples to copy
void OscilloscopeCopyWaveform(int x, int n);

// draw the samples in the waveform buffer onto the frame buffer
// ADC to pixel scaling is applied
void OscilloscopeDrawWaveform(tContext *psContext);

#endif /* OSCILLOSCOPE_H_ */
