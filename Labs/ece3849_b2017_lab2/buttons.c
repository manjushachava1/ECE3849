/*
 * buttons.c
 *
 *  Created on: Aug 12, 2012, modified 9/8/2017
 *      Author: Gene Bogdanov
 */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Mailbox.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Clock.h>
#include <xdc/cfg/global.h>

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "driverlib/adc.h"
#include "buttons.h"

// public global
volatile uint32_t gButtons;	// debounced button state, one per bit in the lowest bits
                            // button is pressed if its bit is 1, not pressed if 0
uint32_t gJoystick[2] = {0}; // joystick coordinates

extern uint32_t gSystemClock; // system clock frequency in Hz

// benchmarking variables
uint32_t gButtonLatency = 0;
uint32_t gButtonResponseTime = 0;
uint32_t gButtonMissedDeadlines = 0;

void ButtonInit(void)
{
    // GPIO PH1 = BoosterPack button 1
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);
    GPIOPinTypeGPIOInput(GPIO_PORTH_BASE, GPIO_PIN_1);
    // GPIO PK6 = BoosterPack button 2
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
    GPIOPinTypeGPIOInput(GPIO_PORTK_BASE, GPIO_PIN_6);
    // GPIO PJ0 and PJ1 = EK-TM4C1294XL buttons 1 and 2
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
    GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
    // GPIO PD4 = BoosterPack Joystick Select button
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    GPIOPinTypeGPIOInput(GPIO_PORTD_BASE, GPIO_PIN_4);

    // analog input AIN13, at GPIO PD2 = BoosterPack Joystick HOR(X)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_2);
    // analog input AIN17, at GPIO PK1 = BoosterPack Joystick VER(Y)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
    GPIOPinTypeADC(GPIO_PORTK_BASE, GPIO_PIN_1);

    // initialize ADC0 peripheral
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    // ADC clock configuration is in the sampling module
    // initialize ADC sampling sequence
    ADCSequenceDisable(ADC0_BASE, 0);
    ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_CH13);                             // Joystick HOR(X)
    ADCSequenceStepConfigure(ADC0_BASE, 0, 1, ADC_CTL_CH17 | ADC_CTL_IE | ADC_CTL_END);  // Joystick VER(Y)
    ADCSequenceEnable(ADC0_BASE, 0);
}

// update the debounced button state gButtons
void ButtonDebounce(uint32_t buttons)
{
	int32_t i, mask;
	static int32_t state[BUTTON_COUNT]; // button state: 0 = released
									    // BUTTON_PRESSED_STATE = pressed
									    // in between = previous state
	for (i = 0; i < BUTTON_COUNT; i++) {
		mask = 1 << i;
		if (buttons & mask) {
			state[i] += BUTTON_STATE_INCREMENT;
			if (state[i] >= BUTTON_PRESSED_STATE) {
				state[i] = BUTTON_PRESSED_STATE;
				gButtons |= mask; // update debounced button state
			}
		}
		else {
			state[i] -= BUTTON_STATE_DECREMENT;
			if (state[i] <= 0) {
				state[i] = 0;
				gButtons &= ~mask;
			}
		}
	}
}

// sample joystick and convert to button presses
void ButtonReadJoystick(void)
{
    ADCProcessorTrigger(ADC0_BASE, 0);          // trigger the ADC sample sequence for Joystick X and Y
    while(!ADCIntStatus(ADC0_BASE, 0, false));  // wait until the sample sequence has completed
    ADCSequenceDataGet(ADC0_BASE, 0, gJoystick);// retrieve joystick data
    ADCIntClear(ADC0_BASE, 0);                  // clear ADC sequence interrupt flag

    // process joystick movements as button presses using hysteresis
    if (gJoystick[0] > JOYSTICK_UPPER_PRESS_THRESHOLD) gButtons |= 1 << 5; // joystick right in position 5
    if (gJoystick[0] < JOYSTICK_UPPER_RELEASE_THRESHOLD) gButtons &= ~(1 << 5);

    if (gJoystick[0] < JOYSTICK_LOWER_PRESS_THRESHOLD) gButtons |= 1 << 6; // joystick left in position 6
    if (gJoystick[0] > JOYSTICK_LOWER_RELEASE_THRESHOLD) gButtons &= ~(1 << 6);

    if (gJoystick[1] > JOYSTICK_UPPER_PRESS_THRESHOLD) gButtons |= 1 << 7; // joystick up in position 7
    if (gJoystick[1] < JOYSTICK_UPPER_RELEASE_THRESHOLD) gButtons &= ~(1 << 7);

    if (gJoystick[1] < JOYSTICK_LOWER_PRESS_THRESHOLD) gButtons |= 1 << 8; // joystick down in position 8
    if (gJoystick[1] > JOYSTICK_LOWER_RELEASE_THRESHOLD) gButtons &= ~(1 << 8);
}

// autorepeat button presses if a button is held long enough
uint32_t ButtonAutoRepeat(void)
{
    static int count[BUTTON_AND_JOYSTICK_COUNT] = {0}; // autorepeat counts
    int i;
    uint32_t mask;
    uint32_t presses = 0;
    for (i = 0; i < BUTTON_AND_JOYSTICK_COUNT; i++) {
        mask = 1 << i;
        if (gButtons & mask)
            count[i]++;     // increment count if button is held
        else
            count[i] = 0;   // reset count if button is let go
        if (count[i] >= BUTTON_AUTOREPEAT_INITIAL &&
                (count[i] - BUTTON_AUTOREPEAT_INITIAL) % BUTTON_AUTOREPEAT_NEXT == 0)
            presses |= mask;    // register a button press due to auto-repeat
    }
    return presses;
}

void ButtonTask(UArg arg1, UArg arg2)
{
    char msg;
    uint32_t timer0_period = TimerLoadGet(TIMER0_BASE, TIMER_A) + 1;

    while (true) {
        Semaphore_pend(ButtonSem, BIOS_WAIT_FOREVER);

        uint32_t t = timer0_period - TimerValueGet(TIMER0_BASE, TIMER_A);
        if (t > gButtonLatency) gButtonLatency = t; // measure latency

        uint32_t gpio_buttons =
                (~GPIOPinRead(GPIO_PORTJ_BASE, 0xff) & (GPIO_PIN_1 | GPIO_PIN_0)) // EK-TM4C1294XL buttons in positions 0 and 1
                | (~GPIOPinRead(GPIO_PORTH_BASE, 0xff) & GPIO_PIN_1) << 1   // BoosterPack button 1 in position 2
                | (~GPIOPinRead(GPIO_PORTK_BASE, 0xff) & GPIO_PIN_6) >> 3   // BoosterPack button 2 in position 3
                | (~GPIOPinRead(GPIO_PORTD_BASE, 0xff) & GPIO_PIN_4);       // BoosterPack Joystick Select button in position 4

        uint32_t old_buttons = gButtons;    // save previous button state
        ButtonDebounce(gpio_buttons);       // Run the button debouncer. The result is in gButtons.
        ButtonReadJoystick();               // Convert joystick state to button presses. The result is in gButtons.
        uint32_t presses = ~old_buttons & gButtons;   // detect button presses (transitions from not pressed to pressed)
        presses |= ButtonAutoRepeat();      // autorepeat presses if a button is held long enough

        if (presses & 1) {        // EK-TM4C1294XL button 1 pressed
            msg = '1';
            Mailbox_post(ButtonMailbox, &msg, BIOS_WAIT_FOREVER);
        }
        if (presses & (1 << 1)) { // EK-TM4C1294XL button 2 pressed
            msg = '2';
            Mailbox_post(ButtonMailbox, &msg, BIOS_WAIT_FOREVER);
        }
        if (presses & (1 << 2)) { // BoosterPack button 1 pressed
            msg = 'A';
            Mailbox_post(ButtonMailbox, &msg, BIOS_WAIT_FOREVER);
        }
        if (presses & (1 << 3)) { // BoosterPack button 2 pressed
            msg = 'B';
            Mailbox_post(ButtonMailbox, &msg, BIOS_WAIT_FOREVER);
        }
        if (presses & (1 << 4)) { // joystick select button pressed
            msg = 'S';
            Mailbox_post(ButtonMailbox, &msg, BIOS_WAIT_FOREVER);
        }
        if (presses & (1 << 5)) { // joystick deflected right
            msg = 'R';
            Mailbox_post(ButtonMailbox, &msg, BIOS_WAIT_FOREVER);
        }
        if (presses & (1 << 6)) { // joystick deflected left
            msg = 'L';
            Mailbox_post(ButtonMailbox, &msg, BIOS_WAIT_FOREVER);
        }
        if (presses & (1 << 7)) { // joystick deflected up
            msg = 'U';
            Mailbox_post(ButtonMailbox, &msg, BIOS_WAIT_FOREVER);
        }
        if (presses & (1 << 8)) { // joystick deflected down
            msg = 'D';
            Mailbox_post(ButtonMailbox, &msg, BIOS_WAIT_FOREVER);
        }

        if (Semaphore_getCount(ButtonSem)) { // next event occurred
            gButtonMissedDeadlines++;
            t = 2 * timer0_period; // timer overflowed since last event
        }
        else t = timer0_period;
        t -= TimerValueGet(TIMER0_BASE, TIMER_A);
        if (t > gButtonResponseTime) gButtonResponseTime = t; // measure response time
    }
}

Void ButtonClock(UArg arg)
{
    Semaphore_post(ButtonSem);
}
