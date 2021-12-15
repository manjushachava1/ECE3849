/*
 * frequency.c
 *
 *  Created on: Dec 7, 2021
 *      Author: ammiera, mchava
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#include "Crystalfontz128x128_ST7735.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/pin_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/adc.h"
#include "inc/tm4c1294ncpdt.h"

// XDCtools Header files
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <xdc/cfg/global.h>

// BIOS Header files
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

// Time Capture ISR
uint32_t prevCount = 0;
uint32_t multiPeriodInterval = 0;
uint32_t accumulatedPeriods = 0;
float avg_frequency;
char period_str[50];
char frequency_str[50];
extern tContext sContext;
extern uint32_t gSystemClock;
uint32_t timerPeriod;

void TimerInit()
{
    // configure GPIO PD0 as timer input T0CCP0 at BoosterPack Connector #1 pin 14
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    GPIOPinTypeTimer(GPIO_PORTD_BASE, GPIO_PIN_0);
    GPIOPinConfigure(GPIO_PD0_T0CCP0);

    TimerDisable(TIMER0_BASE, TIMER_BOTH);
    TimerConfigure(TIMER0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_CAP_TIME_UP);
    TimerControlEvent(TIMER0_BASE, TIMER_A, TIMER_EVENT_POS_EDGE);
    TimerLoadSet(TIMER0_BASE, TIMER_A, 0xffff);   // use maximum load value
    TimerPrescaleSet(TIMER0_BASE, TIMER_A, 0xff); // use maximum prescale value
    TimerIntEnable(TIMER0_BASE, TIMER_CAPA_EVENT);
    TimerEnable(TIMER0_BASE, TIMER_A);
}

// Timer Capture ISR is an interrupt that calculates the period of the ISR
void TimerCapture_ISR(UArg arg1)
{
    // Clear the timer0A Capture interrupt flag
    TimerIntClear(TIMER0_BASE, TIMER_CAPA_EVENT);

    // Use TimerValueGet() to read full 24 bit captured time count
    uint32_t currCount = TimerValueGet(TIMER0_BASE, TIMER_A);

    timerPeriod = (currCount - prevCount) & 0xFFFFFF;

    prevCount = currCount;

}

