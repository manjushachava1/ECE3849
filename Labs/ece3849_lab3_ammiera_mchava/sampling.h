/*
 * sampling.h
 *
 *  Created on: Nov 7, 2021
 *      Author: Manjusha Chava, Alex Miera
 *
 *
 */
#ifndef SAMPLING_H_
#define SAMPLING_H_

//// INCLUDES & DEFINES ////
#include "Crystalfontz128x128_ST7735.h"
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <xdc/cfg/global.h>

#define ADC_BUFFER_SIZE 2048 // size must be a power of 2
#define ADC_BUFFER_WRAP(i) ((i) & (ADC_BUFFER_SIZE - 1)) // index wrapping macro
#define ADC_INT_PRIORITY 0 //priority for ADC1
#define ADC_BITS 12
#define ADC_SAMPLING_RATE 1000000   // [samples/sec] desired ADC sampling rate
#define CRYSTAL_FREQUENCY 25000000  // [Hz] crystal oscillator frequency used to calculate clock rates
#define PLL_FREQUENCY 480000000
#define ADC_OFFSET 2048 //DC average of waveform
#define VIN_RANGE 3.3
#define PIXELS_PER_DIV 20




//// GLOBAL VARIABLES ////
extern volatile uint16_t gADCBuffer[ADC_BUFFER_SIZE]; // circular buffer
extern uint16_t stableADCBuffer[ADC_BUFFER_SIZE];
extern volatile uint32_t gADCErrors; // number of missed ADC deadlines
// extern volatile int32_t gADCBufferIndex; // latest sample index




//// FUNCTION HEADERS ////
void SignalInit(void);
void ADCInit(void);
void DMA_Init(void);
int32_t getADCBufferIndex(void);
int RisingTrigger(int32_t bufferIndex);
void CopySignal(int32_t triggerIndex);
void PWM_HWI1(UArg arg0);
void ADC_HWI0(UArg arg0);

#endif /* SAMPLING_H_ */
