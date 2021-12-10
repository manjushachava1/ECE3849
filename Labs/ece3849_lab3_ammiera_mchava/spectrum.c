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
#define OFFSET 15
#define SCREEN_WIDTH 128
#define FREQ_SCALE 20

const float db_scale = DB_SCALE;




//// GLOBAL VARIABLES ////
size_t buffer_size = KISS_FFT_CFG_SIZE;
kiss_fft_cfg cfg; // Kiss FFT config
static kiss_fft_cpx in[NFFT], out[NFFT]; // complex waveform and spectrum buffers
float out_db[SCREEN_WIDTH];


// imported variables
extern tContext sContext;
char str[50];




//// FUNCTIONS ////
void get_spec_samples(void) {
    static char kiss_fft_cfg_buffer[KISS_FFT_CFG_SIZE]; // Kiss FFT config memory

    int kiss_fft_idx; // index in FFT buffer

    cfg = kiss_fft_alloc(NFFT, 0, kiss_fft_cfg_buffer, &buffer_size); // init Kiss FFT

    for (kiss_fft_idx = 0; kiss_fft_idx < NFFT; kiss_fft_idx++) {    // generate an input waveform
      in[kiss_fft_idx].r = sinf(20*PI*kiss_fft_idx/NFFT); // real part of waveform
      in[kiss_fft_idx].i = 0;                  // imaginary part of waveform
    }
}

void compute_FFT(void) {
    kiss_fft(cfg, in, out); // compute FFT
}

void window_time_dom(void) {
    int i;
    static float w[NFFT]; // window function
    for (i = 0; i < NFFT; i++) {
        // Blackman window
        w[i] = 0.42f
               - 0.5f * cosf(2*PI*i/(NFFT-1))
               + 0.08f * cosf(4*PI*i/(NFFT-1));
        in[i].r = in[i].r * w[i];
    }
}

void convert_to_dB(void) {
    int kiss_fft_idx;

    for (kiss_fft_idx = 0; kiss_fft_idx < SCREEN_WIDTH -1 ; kiss_fft_idx++)
    {
         out_db[kiss_fft_idx] = ((int)(0-roundf(10 * log10f(out[kiss_fft_idx].r * out[kiss_fft_idx].r + out[kiss_fft_idx].i * out[kiss_fft_idx].i)))) + OFFSET;
    }
}

void display_frequency_scale(void){
    // Print volts per division
    snprintf(str, sizeof(str), "%i kHz", FREQ_SCALE);
    GrContextForegroundSet(&sContext, ClrWhite);
    GrStringDraw(&sContext, str, /*length*/-1, /*x*/20, /*y*/3, /*opaque*/false);
}

void display_dB_scale(void) {
    // Print volts per division
    snprintf(str, sizeof(str), "%.1f dB", db_scale);
    GrContextForegroundSet(&sContext, ClrWhite);
    GrStringDraw(&sContext, str, /*length*/-1, /*x*/80, /*y*/3, /*opaque*/
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

//        if (i == 3)
//        {
//            GrLineDrawV(&sContext, (x + (20 * i) + 1), BOTTOM_SCREEN,
//            TOP_SCREEN);
//            GrLineDrawV(&sContext, (x + (20 * i) - 1), BOTTOM_SCREEN,
//            TOP_SCREEN);
//        }
    }
    GrLineDrawV(&sContext, (x + (20 * i) + x), BOTTOM_SCREEN, TOP_SCREEN);

    // draws horizontal grid
    GrLineDrawH(&sContext, LEFTMOST_SCREEN, RIGHTMOST_SCREEN, y);
    for (i = 1; i <= 6; i++)
    {
        GrLineDrawH(&sContext, LEFTMOST_SCREEN, RIGHTMOST_SCREEN,
                    (y + (20 * i)));

//        if (i == 3)
//        {
//            GrLineDrawH(&sContext, LEFTMOST_SCREEN, RIGHTMOST_SCREEN,
//                        (y + (20 * i) + 1));
//            GrLineDrawH(&sContext, LEFTMOST_SCREEN, RIGHTMOST_SCREEN,
//                        (y + (20 * i) - 1));
//        }
    }
    GrLineDrawH(&sContext, LEFTMOST_SCREEN, RIGHTMOST_SCREEN,
                (y + (20 * i) + y));
}

void display_spec_waveform(void) {
    int i;
    for (i = 0; i < LCD_HORIZONTAL_MAX - 1; i++)
    {
        if (i != 0)
        {
            GrLineDraw(&sContext, i - 1, out_db[i - 1], i, out_db[i]);
        }
    }

}
