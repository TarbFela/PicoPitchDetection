//
// Created by The Tragedy of Darth Wise on 12/29/24.
//

#ifndef PITCH_ANALYSIS_H
#define PITCH_ANALYSIS_H

#include <stdint.h>
#include <stdlib.h>

// find the frequency of some audio in Hz using a version of the Yin pitch detection algorithm
// Errors:
//    0: no frequency met the correlation threshhold. This is useful for loud indiscriminate sounds
//    -1: bad arguments
//    -2: malloc failed (damn you, malloc!)
#define CORR_INT_SCALAR 100 // since normalized correlation is between 0 and 1, we adjust so it's between 0 and 100 (or some other scalar)
#define TROUGH_THRESHHOLD 20 // arbitrary. Should do the trick!
int find_frequency(uint16_t *buff, uint32_t size, uint32_t sample_rate, uint32_t min_freq, uint32_t max_freq, uint32_t correlation_threshhold, uint32_t *corrs_dest);

// take a frequency input and output the midi pitch ( C0 = 0 )
// Return values:
//       -2: frequency positive but too low
//       -1: frequency is zero
int frequency_to_pitch( int freq );

void print_note(int pitch);


#endif //PITCH_ANALYSIS_H
