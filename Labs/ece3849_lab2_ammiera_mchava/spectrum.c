/*
 * spectrum.c
 *
 *  Created on: Nov 28, 2021
 *      Author: alexm
 */

//// INCLUDES & DEFINES ////
#include <math.h>
#include "kiss_fft.h"
#include "_kiss_fft_guts.h"
#include "settings.h"
#include "buttons.h"
#include "sampling.h"
#include "settings.h"

#define PI 3.14159265358979f
#define NFFT 1024         // FFT length
#define KISS_FFT_CFG_SIZE (sizeof(struct kiss_fft_state)+sizeof(kiss_fft_cpx)*(NFFT-1))
#define DB_SCALE 20
#define FREQ_SCALE 20
#define VOLTAGE_SCALE 0.1
#define OFFSET 0

const float db_scale = DB_SCALE;




//// GLOBAL VARIABLES ////
size_t buffer_size = KISS_FFT_CFG_SIZE;
kiss_fft_cfg cfg; // Kiss FFT config
static kiss_fft_cpx in[NFFT], out[NFFT]; // complex waveform and spectrum buffers
float out_db[NFFT];

// imported variables
extern tContext sContext;
char str[50];




//// FUNCTIONS ////
void get_spec_samples(void) {
    static char kiss_fft_cfg_buffer[KISS_FFT_CFG_SIZE]; // Kiss FFT config memory

    int ADC_buffer_idx = gADCBufferIndex - NFFT; // index in circular ADC buffer
    int kiss_fft_idx; // index in FFT buffer

    cfg = kiss_fft_alloc(NFFT, 0, kiss_fft_cfg_buffer, &buffer_size); // init Kiss FFT

    for (kiss_fft_idx = 0; kiss_fft_idx < NFFT; kiss_fft_idx++)
    {
        in[kiss_fft_idx].r = gADCBuffer[ADC_BUFFER_WRAP(ADC_buffer_idx)]; // gets current sample from ADC buffer and sets it as real part of the waveform
        in[kiss_fft_idx].i = 0; // imaginary part of the waveform

        ADC_buffer_idx++;
    }
}

void compute_FFT(void) {
    kiss_fft(cfg, in, out); // compute FFT
}

void convert_to_dB(void) {
    int kiss_fft_idx;
    int i;

    for (kiss_fft_idx = 0; kiss_fft_idx < NFFT; kiss_fft_idx++)
    {
         out_db[kiss_fft_idx] = 10 * log10f(out[i].r * out[i].r + out[i].i * out[i].i); ;

        kiss_fft_idx++;
    }
}

void scale_dB_to_grid(void) {
    float sample;
    float sample_buffer[NFFT];
    float fScale;
    int i;

    fScale = (VIN_RANGE * PIXELS_PER_DIV) / ((1 << ADC_BITS) * VOLTAGE_SCALE);

    GrContextForegroundSet(&sContext, ClrYellow); // yellow text

    for (i = 0; i < LCD_HORIZONTAL_MAX; i++)
    {
        sample = out_db[i];
        sample_buffer[i] = ((int) (LCD_VERTICAL_MAX / 2))
                - (int) (fScale * ((int) sample - OFFSET));
        if (i != 0)
        {
            GrLineDraw(&sContext, i - 1, sample_buffer[i - 1], i, sample_buffer[i]);
        }
    }
}

void display_frequency_scale(void) {
    // Print volts per division
    snprintf(str, sizeof(str), "%i kHz", FREQ_SCALE);
    GrContextForegroundSet(&sContext, ClrWhite);
    GrStringDraw(&sContext, str, /*length*/-1, /*x*/5, /*y*/3, /*opaque*/
                 false);
}

void display_dB_scale(void) {
    // Print volts per division
    snprintf(str, sizeof(str), "%.1f dB", db_scale);
    GrContextForegroundSet(&sContext, ClrWhite);
    GrStringDraw(&sContext, str, /*length*/-1, /*x*/60, /*y*/3, /*opaque*/
                 false);
}

// copied and pasted from settings.c, trying to keep everything seperate for now but eventually
// should create a function that takes a color and draws the grid
void display_spec_grid(void) {
    int32_t x = 4;
    int32_t y = 4;
    int i;
    GrContextForegroundSet(&sContext, ClrGreen); // blue lines

    // draws vertical grid
    GrLineDrawV(&sContext, x, BOTTOM_SCREEN, TOP_SCREEN);
    for (i = 1; i <= 6; i++)
    {
        GrLineDrawV(&sContext, (x + (20 * i)), BOTTOM_SCREEN, TOP_SCREEN);

        if (i == 3)
        {
            GrLineDrawV(&sContext, (x + (20 * i) + 1), BOTTOM_SCREEN,
            TOP_SCREEN);
            GrLineDrawV(&sContext, (x + (20 * i) - 1), BOTTOM_SCREEN,
            TOP_SCREEN);
        }
    }
    GrLineDrawV(&sContext, (x + (20 * i) + x), BOTTOM_SCREEN, TOP_SCREEN);

    // draws horizontal grid
    GrLineDrawH(&sContext, LEFTMOST_SCREEN, RIGHTMOST_SCREEN, y);
    for (i = 1; i <= 6; i++)
    {
        GrLineDrawH(&sContext, LEFTMOST_SCREEN, RIGHTMOST_SCREEN,
                    (y + (20 * i)));

        if (i == 3)
        {
            GrLineDrawH(&sContext, LEFTMOST_SCREEN, RIGHTMOST_SCREEN,
                        (y + (20 * i) + 1));
            GrLineDrawH(&sContext, LEFTMOST_SCREEN, RIGHTMOST_SCREEN,
                        (y + (20 * i) - 1));
        }
    }
    GrLineDrawH(&sContext, LEFTMOST_SCREEN, RIGHTMOST_SCREEN,
                (y + (20 * i) + y));
}

void display_spec_waveform(void) {
    int i;

    for (i = 0; i < LCD_HORIZONTAL_MAX; i++)
    {
        if (i != 0)
        {
            GrLineDraw(&sContext, i - 1, out_db[i - 1], i,
                       out_db[i]);
        }
    }
}
