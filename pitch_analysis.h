//
// Created by The Tragedy of Darth Wise on 12/29/24.
//

#ifndef PITCH_ANALYSIS_H
#define PITCH_ANALYSIS_H

#include <stdint.h>
#include <stdlib.h>


typedef struct frequency_analysis {
    uint32_t buff_size;
    uint16_t *audio_buffer;
    uint32_t sample_rate;
    uint32_t correlation_threshhold;
    uint32_t corrs_arr_size;
    uint32_t *correlations_array;
} FREQ_ANALYZER_T;



FREQ_ANALYZER_T *init_freq_analyzer(uint16_t *audio_buff, uint32_t buff_size, uint32_t sample_rate, uint32_t correlation_threshhold);

#define CORR_INT_SCALAR 100 // since normalized correlation is between 0 and 1, we adjust so it's between 0 and 100 (or some other scalar)
#define TROUGH_THRESHHOLD 40 // arbitrary. Should do the trick!

// calculate the yin correlation along the frequency domain, populating the FREQ_ANALYZER_T -> correlations_array
void calculate_yin(FREQ_ANALYZER_T *s);



#define CORR_ROUNDING_AMOUNT 3

// find of the best-match tau value of a yin array from a set of test taus
// Errors:
//    -1: no frequency met the correlation threshhold. This is useful for loud indiscriminate sounds
//    -2: bad arguments
//    -3: malloc failed (damn you, malloc!)
// a freq analyzer struct and an array of "frequencies of interest" (an array of sample-offsets in order of low-offset to high)
int find_dominant_tau(FREQ_ANALYZER_T *s, int n_taus, uint32_t *test_tau_array);

// state machine that schmidtt-triggers itself into a minimum.
// Error code(s):
//      -1: no freq found with schmidtt.
int dominant_freq(FREQ_ANALYZER_T *s);
// same as above with an extra interpolation step for nice precision. // TODO: FIO why broken ):
int dominant_freq_interpolating(FREQ_ANALYZER_T *s);

// slow and not-very-good mean-square algorithm. Has a high offset, for some reason.
uint32_t get_mean_square_volume(FREQ_ANALYZER_T *s);

// simpler. Maybe slower, actually, due to conditional stuff. TODO: optimize with bit twiddling.
uint32_t get_peak_volume(FREQ_ANALYZER_T *s);

// take a frequency input and output the midi pitch ( C0 = 0 )
// Return values:
//       -2: frequency positive but too low
//       -1: frequency is zero
int frequency_to_pitch( int freq );

void print_note(int pitch);

extern const int PITCH_FREQS[12];

#endif //PITCH_ANALYSIS_H
