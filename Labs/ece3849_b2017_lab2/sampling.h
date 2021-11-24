/*
 * sampling.h
 *
 *  Created on: Aug 12, 2012
 *      Author: Gene Bogdanov
 */

#ifndef SAMPLING_H_
#define SAMPLING_H_

#include <stdint.h>

//*****************************************************************************
// ADC waveform sampling functionality

#define ADC_BUFFER_SIZE 2048 // must be a power of 2
#define ADC_BUFFER_WRAP(a) ((a) & (ADC_BUFFER_SIZE - 1)) // wraps the buffer index into range
#define ADC_INT_PRIORITY 0	// ADC interrupt priority: 0 = highest
#define ADC_BITS 12			// # of ADC bits
#define ADC_SAMPLING_RATE 1000000 // [samples/sec] desired ADC sampling rate
#define PLL_FREQUENCY 480000000 // [Hz] PLL frequency (this system uses a divider of 2 after the 480 MHz PLL)

extern volatile int32_t gADCBufferIndex; // index of the newest sample in the ADC buffer
extern volatile uint16_t gADCBuffer[ADC_BUFFER_SIZE]; // ADC buffer
extern volatile uint32_t gADCErrors; // ADC overflow error counter
extern uint32_t gADCSamplingRate; // [Hz] actual ADC sampling rate

// temporary / testing variables
extern volatile unsigned long timer;
extern unsigned short ADC_value;

// ISR transferring samples from the ADC to g_pusADCBuffer
void ADC_ISR(void);

// initialize the ADC and sampling timer
void ADCInit(void);

#endif /* SAMPLING_H_ */
