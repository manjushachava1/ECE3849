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

#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/pin_map.h"

// Time Capture ISR
uint32_t prevCount = 0;
uint32_t timerPeriod = 0;
uint32_t multiPeriodInterval = 0;
uint32_t accumulatedPeriods = 0;
float avg_frequency;

void TimerInit()
{
    // configure GPIO PD0 as timer input T0CCP0 at BoosterPack Connector #1 pin 14
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    GPIOPinTypeTimer(GPIO_PORTD_BASE, GPIO_PIN_0);
    GPIOPinConfigure(GPIO_PD0_T0CCP0);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    TimerDisable(TIMER0_BASE, TIMER_BOTH);
    TimerConfigure(TIMER0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_CAP_TIME_UP);
    TimerControlEvent(TIMER0_BASE, TIMER_A, TIMER_EVENT_POS_EDGE);
    TimerLoadSet(TIMER0_BASE, TIMER_A, 0xffff);   // use maximum load value
    TimerPrescaleSet(TIMER0_BASE, TIMER_A, 0xff); // use maximum prescale value
    TimerIntEnable(TIMER0_BASE, TIMER_CAPA_EVENT);
    TimerEnable(TIMER0_BASE, TIMER_A);
}


// Timer Capture ISR is an interrupt that calculates the period of the ISR
void TimerCapture_ISR(UArg arg0){
    // Clear the timer0A Capture interrupt flag
    TIMER0_ICR_R = TIMER_ICR_CAECINT;

    // Use TimerValueGet() to read full 24 bit captured time count
    uint32_t currCount = TimerValueGet(TIMER0_BASE, TIMER_A);

    timerPeriod = (currCount - prevCount) & 0xFFFFFF;

    prevCount = currCount;

    multiPeriodInterval += timerPeriod;
    accumulatedPeriods++;
}

// Clock to post to frequency task
void freq_clock(UArg arg1){
    Semaphore_post(FrequencySem); // to buttons
}

// Determines the average frequency as the ratio of the number of accumulated periods
// to the accumulated interval, converted from clock cycles to seconds
void frequency_task(UArg arg1, UArg arg2) {
    IArg key;
    uint32_t accu_int, accu_count;

    while (true) {
        Semaphore_pend(FrequencySem, BIOS_WAIT_FOREVER); // from clock

        // Protect global shared data
        key = GateHwi_enter(gateHwi0);
        // Retrieve global data and reset
        accu_int = multiPeriodInterval;
        accu_count = accumulatedPeriods;
        // Reset globals back to 0
        multiPeriodInterval = 0;
        accumulatedPeriods = 0;
        GateHwi_leave(gateHwi0, key);

        // determine average frequency
        float avg_period = (float)accu_int/accu_count;
        avg_frequency = 1/(avg_period) * (float)gSystemClock;

    }
}
