/*
 * sampling.c
 *
 *  Created on: Aug 12, 2012
 *      Author: Gene Bogdanov
 */
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

volatile int32_t gADCBufferIndex = ADC_BUFFER_SIZE - 1;
volatile uint16_t gADCBuffer[ADC_BUFFER_SIZE];
volatile uint32_t gADCErrors;
uint32_t gADCSamplingRate = 0;

void ADC_ISR(void)
{
//	static const short sin_samples[32] = {512, 579, 643, 702, 753, 796, 827, 847, 853, 847, 827, 796,
//			753, 702, 643, 579, 512, 445, 381, 322, 271, 228, 197, 177, 171, 177, 197, 228, 271, 322, 381, 445
//	};
    ADC1_ISC_R = ADC_ISC_IN0; // clear interrupt flag (must be done early)
    if (ADC1_OSTAT_R & ADC_OSTAT_OV0) { // check for ADC FIFO overflow
		gADCErrors++;	// count errors
		ADC1_OSTAT_R = ADC_OSTAT_OV0; // clear overflow condition
	}
	// grab and save the A/D conversion result
	gADCBuffer[
	           gADCBufferIndex = ADC_BUFFER_WRAP(gADCBufferIndex + 1)
	           ] = ADC1_SSFIFO0_R;
//	sin_samples[gADCBufferIndex & 0x1f];
//	ADC1_SSFIFO0_R; // read ADC result and drop on floor
//	ADC_value;
//	timer++;
}

void ADCInit(void)
{
	// initialize ADC peripherals
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC1);
	// ADC clock
	uint32_t pll_divisor = (PLL_FREQUENCY - 1) / (16 * ADC_SAMPLING_RATE) + 1; // round divisor up
	gADCSamplingRate = PLL_FREQUENCY / (16 * pll_divisor); // actual sampling rate may differ from ADC_SAMPLING_RATE
	ADCClockConfigSet(ADC0_BASE, ADC_CLOCK_SRC_PLL | ADC_CLOCK_RATE_FULL, pll_divisor); // only ADC0 has PLL clock divisor control
	ADCClockConfigSet(ADC1_BASE, ADC_CLOCK_SRC_PLL | ADC_CLOCK_RATE_FULL, pll_divisor);
	// use analog input AIN3, at GPIO PE0, which is BoosterPack 1 header B1 (2nd from left) pin 3
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_0);

	ADCSequenceDisable(ADC1_BASE, 0);
	ADCSequenceConfigure(ADC1_BASE, 0, ADC_TRIGGER_ALWAYS, 0);
	ADCSequenceStepConfigure(ADC1_BASE, 0, 0, ADC_CTL_IE | ADC_CTL_END | ADC_CTL_CH3);
	ADCSequenceEnable(ADC1_BASE, 0);
	ADCIntEnable(ADC1_BASE, 0);

	// initialize timer to trigger the ADC
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
	TimerDisable(TIMER1_BASE, TIMER_BOTH);
	TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
	TimerControlTrigger(TIMER1_BASE, TIMER_A, true);
}
