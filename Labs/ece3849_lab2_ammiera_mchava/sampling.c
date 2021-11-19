/*
 * sampling.h
 *
 *  Created on: Nov 7, 2021
 *      Author: Manjusha Chava, Alex Miera
 *
 *
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




//// GLOBAL VARIABLES ////
volatile uint32_t gADCErrors; // number of missed ADC deadlines
volatile int32_t gADCBufferIndex = ADC_BUFFER_SIZE - 1; // latest sample index
volatile uint16_t gADCBuffer[ADC_BUFFER_SIZE];
uint32_t gADCSamplingRate = 0;
uint16_t stableADCBuffer[ADC_BUFFER_SIZE];
extern uint32_t gSystemClock; // [Hz] system clock frequency




//// FUNCTIONS ////

// METHOD CALL: main.c
// DESCRIPTION: initializes ADC hardware
// INPUTS: void
// OUTPUTS: void
// AUTHOR: professor
// REVISION HISTORY: NA
// NOTES: NA
// TODO: NA
void ADCInit(void) {

    // initialize ADC peripherals

    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC1);

    // ADC clock
    uint32_t pll_divisor = (PLL_FREQUENCY - 1) / (16 * ADC_SAMPLING_RATE) + 1; // round divisor up
    gADCSamplingRate = PLL_FREQUENCY / (16 * pll_divisor); // actual sampling rate may differ from ADC_SAMPLING_RATE
    ADCClockConfigSet(ADC0_BASE, ADC_CLOCK_SRC_PLL | ADC_CLOCK_RATE_FULL,
                     pll_divisor); // only ADC0 has PLL clock divisor control
    ADCClockConfigSet(ADC1_BASE, ADC_CLOCK_SRC_PLL | ADC_CLOCK_RATE_FULL,
                      pll_divisor);

    // use analog input AIN3, at GPIO PE0, which is BoosterPack 1 header B1 (2nd from left) pin 3
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_0);
    ADCSequenceDisable(ADC1_BASE, 0);
    ADCSequenceConfigure(ADC1_BASE, 0, ADC_TRIGGER_ALWAYS, 0);
    ADCSequenceStepConfigure(ADC1_BASE, 0, 0,
                             ADC_CTL_IE | ADC_CTL_END | ADC_CTL_CH3);

    ADCSequenceEnable(ADC1_BASE, 0);
    ADCIntEnable(ADC1_BASE, 0);
    IntPrioritySet(INT_ADC1SS0, ADC_INT_PRIORITY << (8 - NUM_PRIORITY_BITS));
    IntEnable(INT_ADC1SS0);

    // initialize timer to trigger the ADC
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
    TimerDisable(TIMER1_BASE, TIMER_BOTH);
    TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
    TimerControlTrigger(TIMER1_BASE, TIMER_A, true);

}

// METHOD CALL: RTOS?
// DESCRIPTION: samples analog signal
// INPUTS:
//      * arg0 - ?
// OUTPUTS: void
// AUTHOR: mchava, ammiera
// REVISION HISTORY: 11/19/2021
// NOTES:
// TODO:
//      * Need to figure out what is calling this ISR
//      * Need to figure out what arg0 is for
//      * Testing
void ISR0_ADC(UArg arg0) {

    ADC1_ISC_R = ADC_ISC_IN0; // clears ADC interrupt flag
    if (ADC1_OSTAT_R & ADC_OSTAT_OV0)
    { // check for ADC FIFO overflow
        gADCErrors++;                   // count errors
        ADC1_OSTAT_R = ADC_OSTAT_OV0;   // clear overflow condition
    }
    gADCBuffer[gADCBufferIndex = ADC_BUFFER_WRAP(gADCBufferIndex + 1)] =
    ADC1_SSFIFO0_R;         // read sample from the ADC1 sequence 0 FIFO
}

