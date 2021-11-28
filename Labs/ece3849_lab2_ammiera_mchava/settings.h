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



#endif /* SETTINGS_H_ */
