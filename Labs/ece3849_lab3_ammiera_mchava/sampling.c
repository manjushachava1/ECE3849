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
#include <stdio.h>
#include <math.h>
#include "Crystalfontz128x128_ST7735.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "inc/tm4c1294ncpdt.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "driverlib/udma.h"
#include "driverlib/fpu.h"
#include "driverlib/sysctl.h"
#include "driverlib/pwm.h"
#include "driverlib/pin_map.h"
#include "driverlib/adc.h"
#include "sampling.h"
#include "string.h"
#include "fifo.h"
#include "settings.h"
#include "buttons.h"
#include "sampling.h"

// XDCtools Header files
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <xdc/cfg/global.h>

// BIOS Header files
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Event.h>

// PWM
#include "driverlib/pwm.h"
#include "inc/tm4c1294ncpdt.h"
#include "audio_waveform.h"

#define PWM_PERIOD 258




//// GLOBAL VARIABLES ////
volatile uint32_t gADCErrors; // number of missed ADC deadlines
// volatile int32_t gADCBufferIndex = ADC_BUFFER_SIZE - 1; // latest sample index
volatile uint16_t gADCBuffer[ADC_BUFFER_SIZE];
uint32_t gADCSamplingRate = 0;
uint16_t stableADCBuffer[ADC_BUFFER_SIZE];
extern uint32_t gSystemClock; // [Hz] system clock frequency

// DMA
#pragma DATA_ALIGN(gDMAControlTable, 1024) // address alignment required
tDMAControlTable gDMAControlTable[64];     // uDMA control table (global)
volatile bool gDMAPrimary = true; // is DMA occurring in the primary channel?

// PWM
uint32_t gPWMSample = 0;            // PWM sample counter
uint32_t gSamplingRateDivider = 60; // (int) floor((gSystemClock/PWM_PERIOD)/(AUDIO_SAMPLING_RATE)); // sampling rate divider


//// FUNCTIONS ////

// METHOD CALL: main.c
// DESCRIPTION: initializes signaling hardware
// INPUTS: void
// OUTPUTS: void
// AUTHOR: professor
// REVISION HISTORY: NA
// NOTES: NA
// TODO: NA
void SignalInit(void)
{
    // configure M0PWM5, at GPIO PG1
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
                    // GPIO_PIN_TYPE_STD);
    GPIOPinTypePWM(GPIO_PORTG_BASE, GPIO_PIN_1); // PG1 = M0PWM5
    GPIOPinConfigure(GPIO_PG1_M0PWM5);
    GPIOPadConfigSet(GPIO_PORTG_BASE, GPIO_PIN_1,
    GPIO_STRENGTH_2MA,
                     GPIO_PIN_TYPE_STD);

    // configure M0PWM2, at GPIO PF2, which is BoosterPack 1 header C1 (2nd from right) pin 2
    // configure M0PWM3, at GPIO PF3, which is BoosterPack 1 header C1 (2nd from right) pin 3
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3); // PF2 = M0PWM2, PF3 = M0PWM3
    GPIOPinConfigure(GPIO_PF2_M0PWM2);
    GPIOPinConfigure(GPIO_PF3_M0PWM3);
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3,
    GPIO_STRENGTH_2MA,
                     GPIO_PIN_TYPE_STD);

    // configure the PWM0 peripheral, gen 2, output 5
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
    PWMClockSet(PWM0_BASE, PWM_SYSCLK_DIV_1); // use system clock without division
    PWMGenConfigure(PWM0_BASE, PWM_GEN_2,
    PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, PWM_PERIOD);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5,
                     roundf((float) PWM_PERIOD * 0.5f)); // 50% duty cycle
    PWMOutputState(PWM0_BASE, PWM_OUT_5_BIT, true);
    PWMGenEnable(PWM0_BASE, PWM_GEN_2);

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

    PWMGenIntTrigEnable(PWM0_BASE, PWM_GEN_2, PWM_INT_CNT_ZERO);
}

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
    gADCSamplingRate = 2000000; //PLL_FREQUENCY / (16 * pll_divisor); // actual sampling rate may differ from ADC_SAMPLING_RATE
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
//    ADCIntEnable(ADC1_BASE, 0);
//    IntPrioritySet(INT_ADC1SS0, ADC_INT_PRIORITY << (8 - NUM_PRIORITY_BITS));
//    IntEnable(INT_ADC1SS0);

    // initialize timer to trigger the ADC
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
    TimerDisable(TIMER1_BASE, TIMER_BOTH);
    TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
    TimerControlTrigger(TIMER1_BASE, TIMER_A, true);
}

// METHOD CALL: main.c
// DESCRIPTION: initializes DMA controller
// INPUTS: void
// OUTPUTS: void
// AUTHOR: professor
// REVISION HISTORY: NA
// NOTES: NA
// TODO: NA
void DMA_Init(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    uDMAEnable();
    uDMAControlBaseSet(gDMAControlTable);

    uDMAChannelAssign(UDMA_CH24_ADC1_0); // assign DMA channel 24 to ADC1 sequence 0
    uDMAChannelAttributeDisable(UDMA_SEC_CHANNEL_ADC10, UDMA_ATTR_ALL);

    // primary DMA channel = first half of the ADC buffer
    uDMAChannelControlSet(UDMA_SEC_CHANNEL_ADC10 | UDMA_PRI_SELECT,
        UDMA_SIZE_16 | UDMA_SRC_INC_NONE | UDMA_DST_INC_16 | UDMA_ARB_4);
    uDMAChannelTransferSet(UDMA_SEC_CHANNEL_ADC10 | UDMA_PRI_SELECT,
        UDMA_MODE_PINGPONG, (void*)&ADC1_SSFIFO0_R,
        (void*)&gADCBuffer[0], ADC_BUFFER_SIZE/2);

    // alternate DMA channel = second half of the ADC buffer
    uDMAChannelControlSet(UDMA_SEC_CHANNEL_ADC10 | UDMA_ALT_SELECT,
        UDMA_SIZE_16 | UDMA_SRC_INC_NONE | UDMA_DST_INC_16 | UDMA_ARB_4);
    uDMAChannelTransferSet(UDMA_SEC_CHANNEL_ADC10 | UDMA_ALT_SELECT,
        UDMA_MODE_PINGPONG, (void*)&ADC1_SSFIFO0_R,
        (void*)&gADCBuffer[ADC_BUFFER_SIZE/2], ADC_BUFFER_SIZE/2);

    uDMAChannelEnable(UDMA_SEC_CHANNEL_ADC10);

    ADCSequenceDMAEnable(ADC1_BASE, 0); // enable DMA for ADC1 sequence 0
    ADCIntEnableEx(ADC1_BASE, ADC_INT_DMA_SS0); // enable ADC1 sequence 0 DMA interrupt
    // see https://software-dl.ti.com/simplelink/esd/simplelink_msp432e4_sdk/2.30.00.14/docs/driverlib/msp432e4/html/group__adc__api.html#gae1acb71806ec0cec55290c938fa8cc02
    // for information on ADCIntEnableEx
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
void ADC_HWI0(UArg arg0) {

    ADCIntClearEx(ADC1_BASE, ADC_INT_DMA_SS0); // clear the ADC1 sequence 0 DMA interrupt flag

    if (uDMAChannelModeGet(UDMA_SEC_CHANNEL_ADC10 | UDMA_PRI_SELECT) ==
            UDMA_MODE_STOP) { // checks if primary channel is being used

        // checks the primary DMA channel for end of transfer, and restart if needed
        uDMAChannelTransferSet(UDMA_SEC_CHANNEL_ADC10 | UDMA_PRI_SELECT,
                               UDMA_MODE_PINGPONG, (void*)&ADC1_SSFIFO0_R,
                               (void*)&gADCBuffer[ADC_BUFFER_SIZE/2], ADC_BUFFER_SIZE/2); // restart the primary channel (same as setup)
        gDMAPrimary = false; // DMA is currently occurring in the alternate buffer
    }


    if (uDMAChannelModeGet(UDMA_SEC_CHANNEL_ADC10 | UDMA_ALT_SELECT) ==
            UDMA_MODE_STOP) { // checks if alternate channel is being used

        // checks the alternate DMA channel for end of transfer, and restart if needed
        uDMAChannelTransferSet(UDMA_SEC_CHANNEL_ADC10 | UDMA_ALT_SELECT,
                               UDMA_MODE_PINGPONG, (void*)&ADC1_SSFIFO0_R,
                               (void*)&gADCBuffer[ADC_BUFFER_SIZE/2], ADC_BUFFER_SIZE/2); // restart the secondary channel (same as setup)

        gDMAPrimary = true; // DMA is currently occurring in the primary buffer
    }

    // The DMA channel may be disabled if the CPU is paused by the debugger.
    if (!uDMAChannelIsEnabled(UDMA_SEC_CHANNEL_ADC10)) {
        uDMAChannelEnable(UDMA_SEC_CHANNEL_ADC10);  // re-enable the DMA channel
    }
}

void PWM_HWI1(UArg arg0)
{
    PWMGenIntClear(PWM0_BASE, PWM_GEN_2, PWM_INT_GEN_2); // clear PWM interrupt flag

    int i = (gPWMSample++) / gSamplingRateDivider; // waveform sample index
    PWM0_2_CMPB_R = 1 + gWaveform[i]; // write directly to the PWM compare B register
    if (i >= gWaveformSize) { // if at the end of the waveform array
        PWMIntDisable(PWM0_BASE, PWM_INT_GEN_2); // disable these interrupts
        gPWMSample = 0; // reset sample index so the waveform starts from the beginning
    }
}

// METHOD CALL: main.c
// DESCRIPTION: finds oscilloscope trigger
// INPUTS: void
// OUTPUTS:
//      * triggerIndex - resulting trigger index based on search algorithm
// AUTHOR: ammiera
// REVISION HISTORY: 11/12/2021
// NOTES: NA
// TODO: NA
int RisingTrigger(int32_t bufferIndex)
{
// Step 1
    int triggerIndex = bufferIndex - HALF_SCREEN_IDX; // half screen width; don't use a magic number;

    // Step 2
    int triggerSearchStop = triggerIndex - (ADC_BUFFER_SIZE / 2);
    for (; triggerIndex > triggerSearchStop; triggerIndex--)
    {
        // if trigger is found
        if (gADCBuffer[ADC_BUFFER_WRAP(triggerIndex)] >= ADC_OFFSET && /* checks current sample */
        gADCBuffer[ADC_BUFFER_WRAP(triggerIndex) + 1] < ADC_OFFSET /* checks next older sample */)
            break;
    }

    // Step 3
    if (triggerIndex == triggerSearchStop) // for loop ran to the end
        triggerIndex = bufferIndex - HALF_SCREEN_IDX; // reset trigger search index back to how it was initialized

    return triggerIndex;
}

// METHOD CALL: main.c
// DESCRIPTION: copies signal starting from and ending 1/2 way behind the trigger index
// INPUTS:
//      * sContext - for LCD graphics display
//      * triggerIndex - index of trigger from RisingTrigger()
// OUTPUTS: void
// AUTHOR: ammiera
// REVISION HISTORY: 11/12/2021
// NOTES: NA
// TODO: NA
void CopySignal(int32_t triggerIndex)
{
    int i = triggerIndex - (ADC_BUFFER_SIZE / 2); // indexes samples
    int j = 0; // keeps track of local buffer index

    for (; i < triggerIndex; i++)
    {
        stableADCBuffer[j] = gADCBuffer[ADC_BUFFER_WRAP(i)]; // gets current sample
        j++;
    }
}

int32_t getADCBufferIndex(void)
{
    int32_t index;
    IArg key;

    key = GateHwi_enter(gateHwi0);
    if (gDMAPrimary) {  // DMA is currently in the primary channel
        index = ADC_BUFFER_SIZE/2 - 1 -
                uDMAChannelSizeGet(UDMA_SEC_CHANNEL_ADC10 | UDMA_PRI_SELECT);
    }
    else {              // DMA is currently in the alternate channel
        index = ADC_BUFFER_SIZE - 1 -
                uDMAChannelSizeGet(UDMA_SEC_CHANNEL_ADC10 | UDMA_ALT_SELECT);
    }
    GateHwi_leave(gateHwi0, key);

    return index;
}
