/*
 * settings.h
 *
 *  Created on: Nov 15, 2021
 *      Author: alexm
 */

#ifndef SETTINGS_H_
#define SETTINGS_H_

//// INCLUDES & DEFINES ////
#include "Crystalfontz128x128_ST7735.h"

#define PWM_FREQUENCY 20000 // PWM frequency = 20 kHz
#define HALF_SCREEN_IDX 64 // LCD on booster pack is 128x128-pixel
// each pixel contains 1 data point
// 1/2 screen is 1/2 of 128
#define BOTTOM_SCREEN 0
#define TOP_SCREEN 128
#define LEFTMOST_SCREEN 0
#define RIGHTMOST_SCREEN 128
#define TIME_SCALE_STEPS 12     // # of different time scales
#define VOLTAGE_SCALE_STEPS 4   // # of different voltage scales
#define FRAME_SIZE_X LCD_HORIZONTAL_MAX // screen dimensions
#define FRAME_SIZE_Y LCD_VERTICAL_MAX
#define ADC_RANGE 3.3

//// GLOBAL VARIABLES ////
extern uint32_t count_unloaded;
extern uint32_t count_loaded;
extern float cpu_load;
extern float gVoltageScale;
extern float const cVoltageScales[];
extern const char S1;
extern const char S2;
extern const char NA;




//// FUNCTION HEADERS ////
float GetVoltageScale(void); // gets
void DrawGrid(void);
void DrawTriggerSlope(void);
void WriteTimeScale(int timeScale);
void WriteVoltageScale(float voltageScale);
void ADCSampleScaling(float voltageScale);
void DrawFrame(void);
uint32_t WriteCPULoad(int flag);
void OscilloscopeTimeScale(void);
void OscilloscopeVoltageScale(void);
void OscilloscopeTriggerLevel(void);
void UserInput(char button);

#endif /* SETTINGS_H_ */
