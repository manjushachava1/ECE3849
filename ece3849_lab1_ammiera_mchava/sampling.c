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
// initializes PWM hardware
void SignalInit(void)
{
    // configure M0PWM2, at GPIO PF2, which is BoosterPack 1 header C1 (2nd from right) pin 2
    // configure M0PWM3, at GPIO PF3, which is BoosterPack 1 header C1 (2nd from right) pin 3
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3); // PF2 = M0PWM2, PF3 = M0PWM3
    GPIOPinConfigure(GPIO_PF2_M0PWM2);
    GPIOPinConfigure(GPIO_PF3_M0PWM3);
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3,
    GPIO_STRENGTH_2MA,
                     GPIO_PIN_TYPE_STD);

    // configure the PWM0 peripheral, gen 1, outputs 2 and 3
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
    PWMClockSet(PWM0_BASE, PWM_SYSCLK_DIV_1); // use system clock without division
    PWMGenConfigure(PWM0_BASE, PWM_GEN_1,
    PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1,
                    roundf((float) gSystemClock / PWM_FREQUENCY));
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2,
                     roundf((float) gSystemClock / PWM_FREQUENCY * 0.4f)); // 40% duty cycle
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3,
                     roundf((float) gSystemClock / PWM_FREQUENCY * 0.4f));
    PWMOutputState(PWM0_BASE, PWM_OUT_2_BIT | PWM_OUT_3_BIT, true);
    PWMGenEnable(PWM0_BASE, PWM_GEN_1);

    // initialize timer 3 in one-shot mode for polled timing
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER3);
    TimerDisable(TIMER3_BASE, TIMER_BOTH);
    TimerConfigure(TIMER3_BASE, TIMER_CFG_ONE_SHOT);
    TimerLoadSet(TIMER3_BASE, TIMER_A, (gSystemClock) / 100); // 10ms sec interval
}

// initializes ADC hardware
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

// gets data from the ADC ISR
void ADC_ISR(void) {

    ADC1_ISC_R = ADC_ISC_IN0; // clears ADC interrupt flag
    if (ADC1_OSTAT_R & ADC_OSTAT_OV0)
    { // check for ADC FIFO overflow
        gADCErrors++;                   // count errors
        ADC1_OSTAT_R = ADC_OSTAT_OV0;   // clear overflow condition
    }
    gADCBuffer[gADCBufferIndex = ADC_BUFFER_WRAP(gADCBufferIndex + 1)] =
    ADC1_SSFIFO0_R;         // read sample from the ADC1 sequence 0 FIFO
}

// search for rising edge trigger
int RisingTrigger(void)
{
// Step 1
    int triggerSearchIdx = gADCBufferIndex - HALF_SCREEN_IDX; // half screen width; don't use a magic number;

    // Step 2
    int triggerSearchStop = triggerSearchIdx - (ADC_BUFFER_SIZE / 2);
    for (; triggerSearchIdx > triggerSearchStop; triggerSearchIdx--)
    {
        // if trigger is found
        if (gADCBuffer[ADC_BUFFER_WRAP(triggerSearchIdx)] >= ADC_OFFSET && /* checks current sample */
        gADCBuffer[ADC_BUFFER_WRAP(triggerSearchIdx) + 1] < ADC_OFFSET /* checks next older sample */)
            break;
    }

    // Step 3
    if (triggerSearchIdx == triggerSearchStop) // for loop ran to the end
        triggerSearchIdx = gADCBufferIndex - HALF_SCREEN_IDX; // reset trigger search index back to how it was initialized

    return triggerSearchIdx;
}

// copies signal data from the ADC buffer to a stable buffer
void CopySignal(tContext sContext, int triggerIndex)
{
    int i = triggerIndex - (ADC_BUFFER_SIZE / 2); // indexes samples
    int j = 0; // keeps track of local buffer index

    for (; i < triggerIndex; i++)
    {
        stableADCBuffer[j] = gADCBuffer[ADC_BUFFER_WRAP(i)]; // gets current sample
        j++;
    }
}
