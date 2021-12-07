/*
 * spectrum.h
 *
 *  Created on: Nov 28, 2021
 *      Author: alexm
 */

#ifndef SPECTRUM_H_
#define SPECTRUM_H_

//// FUNCTIONS ////
void get_spec_samples(void);
void window_time_dom(void);
void compute_FFT(void);
void convert_to_dB(void);
void display_frequency_scale(void);
void display_dB_scale(void);
void display_spec_grid(void);
void display_spec_waveform(void);

#endif /* SPECTRUM_H_ */
