//
// Created by The Tragedy of Darth Wise on 12/29/24.
//

#include "pitch_analysis.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>




// find the frequency of some audio in Hz using a version of the Yin pitch detection algorithm
// Errors:
//    0: no frequency met the correlation threshhold. This is useful for loud indiscriminate sounds
//    -1: bad arguments
//    -2: malloc failed (damn you, malloc!)
// TODO: make it return a frequency
// TODO: re-organize the passed info into a typedef struct
int find_frequency(uint16_t *buff, uint32_t size, uint32_t sample_rate, uint32_t min_freq, uint32_t max_freq, uint32_t correlation_threshhold, uint32_t *corrs_dest) {
    uint32_t half_size = size / 2;
    int ret_val = -1;
    // check for bad inputs
    if(min_freq < half_size / sample_rate || max_freq > sample_rate || max_freq < min_freq) return -1;

    uint32_t tau;
    uint32_t min_tau = 5;//sample_rate / max_freq;

    // note: uint32_t goes up to 4.3 billion. The highest correlations
    // I've seen so far with buffer sizes this big are around 5 million,
    // so this could be a hazard but seems safe for now.

    uint32_t *corrs;
    corrs = (uint32_t *) malloc( (half_size + 1) * sizeof(uint32_t)  );

    uint32_t cum_avg = 1;

    for( tau = 0; tau < half_size; tau++) {
        corrs[tau] = 0;
        // CORRELATION CALCULATION
        for( int sample = 0; sample < half_size; sample ++) {
            int diff = (int)(buff[sample] - buff[sample+tau]);
            uint32_t diff_squared = diff*diff;
            corrs[tau] += diff_squared;
        }
        // NORMALIZATION (averaging)
        if(tau == 0) cum_avg = corrs[tau];
        else {
            cum_avg = (cum_avg * (tau - 1) + corrs[tau]) / tau;
            corrs[tau] = CORR_INT_SCALAR * corrs[tau] / cum_avg;
            //printf("freq: %7.0d, cumavg: %10.0d, corr_norm: %7.0d", (int)(sample_rate/tau), cum_avg, corrs[tau]);
        }
        corrs_dest[tau] = corrs[tau]; // TODO: REMOVE AFTER DEBUG, as well as argument
    }

    // FIND THE LOWEST FREQUENCY TROUGH
    // Considerations:
    // (1) High Tau = Low Frequency
    // (2) Audio with a frequency "f" will produce peaks at corrs[f], corrs[2f], corrs[3f]...
    // (3) The actual corrs curve is **not smooth**
    // Method: A trough is characterized by falling and then rising
    // (1) We will start from our highest tau (lowest frequency) and increment up
    // (2) We will keep track of the running minimum
    // (3) If we're going down a trough, we will be very close to our most recent minimum
    // (4) If we're going up a trough, we will be further from our minimum
    // (5) So if our corrs[tau] is much bigger than our minimum, we'll know we reached the local minimum a little while back
    // (6) it's also good to check that our local min was withing the correlation threshhold in the first place
    uint32_t corrs_min = CORR_INT_SCALAR; //biggest (good) normalized corr value
    int corrs_min_index = -1; // -1 is a "nothing found" marker
    for( tau = half_size - 1; tau >= min_tau; tau--) { //big tau --> small tau :: low freq --> high freq
        if(corrs[tau] > CORR_INT_SCALAR) continue; // skip bad values
        if(corrs[tau] < corrs_min && corrs[tau] < correlation_threshhold) { //min update (only on low enough values)
            corrs_min = corrs[tau];
            corrs_min_index = tau;
        } else if(corrs[tau] - TROUGH_THRESHHOLD > corrs_min) break; // if we've gone back up enough, break
    }
    int tau_out = corrs_min_index;

    free(corrs); // oopsies don't want a memory leak now, do we?

    if(tau_out == -1) return 0;
    return tau_out;
}


// frequencies of C0 to B0
const int PITCH_FREQS[12] = {  16, 17, 18, 19,
                               21, 22, 23, 25,
                               26, 28, 29, 31 };
// take a frequency input and output the midi pitch ( C0 = 0 )
// Return values:
//       -2: frequency positive but too low
//       -1: frequency is zero
int frequency_to_pitch( int freq) {
    if(freq == 0) return -1;
    else if(freq < PITCH_FREQS[0]) return -2;

    int pitch = 0;

    // divide frequency by 2 until we get to the range of acceptable frequencies
    while(freq > PITCH_FREQS[11]) {freq >>= 1; pitch += 12;}

    //iterate through list of frequencies and find the minimum
    // note: would a binary search be faster? Yes. How much time does this take? Not enough to matter.


    //arbitrary for code density
    int min_diff = 100;
    int ii = -1;
    for( int i =0; i<12; i++) {
        int diff = PITCH_FREQS[i] - freq;
        diff = (diff < 0 ) ? diff * -1 : diff; // absolute value
        if(min_diff > diff) {
            min_diff = diff;
            ii = i;
        }
    }
    if(ii == -1) return -3;

    pitch += ii;

    return pitch;

}


const char print_note_strings[12][3] = {
     "C\0","C#\0", "D\0","D#\0", "E\0","F\0","F#\0", "G\0","G#\0", "A\0","A#\0", "B\0"
};
void print_note(int pitch) {
    if(pitch < 1) printf("NOTE: Bad print_note() input...\n");
    else printf("NOTE: %s  (%d)\n",print_note_strings[pitch%12], pitch/12);

}
