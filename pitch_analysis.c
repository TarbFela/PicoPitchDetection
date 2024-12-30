//
// Created by The Tragedy of Darth Wise on 12/29/24.
//

#include "pitch_analysis.h"

#include <stdint.h>
#include <stdlib.h>




// find the frequency of some audio in Hz using a version of the Yin pitch detection algorithm
// Errors:
//    0: no frequency met the correlation threshhold. This is useful for loud indiscriminate sounds
//    -1: bad arguments
//    -2: malloc failed (damn you, malloc!)

int find_frequency(uint16_t *buff, uint32_t size, uint32_t sample_rate, uint32_t min_freq, uint32_t max_freq, uint32_t correlation_threshhold) {
    // check for bad inputs
    if(min_freq < size * 0.5 / sample_rate || max_freq > sample_rate || max_freq < min_freq) return -1;

    uint32_t min_tau = sample_rate / max_freq;
    uint32_t max_tau = sample_rate / min_freq;
    uint32_t tau;

    // I think the compiler would handle this optimization but I'd rather be sure...
    uint32_t half_size = size / 2;

    // note: uint32_t goes up to 4.3 billion. The highest correlations
    // I've seen so far with buffer sizes this big are around 5 million,
    // so this could be a hazard but seems safe for now.
    uint32_t *corrs;
    corrs = (uint32_t *) malloc( (max_freq - min_freq) * sizeof(uint32_t) );

    uint32_t cum_avg = 1;


    for(int i = 0, tau = min_tau ; tau < max_tau; i++, tau++) {
        corrs[i] = 0;  // new tau value, new associated correlation
        for(int t = 0; t < half_size; t++) {
            uint32_t diff = (buff[t] - buff[t + tau]);
            corrs[tau] += diff * diff;
        // a cumulative average...
        // avg = sum of corrs / tau
        // by adding another corr, we must say:
        // old avg = sum of old corrs / (tau - 1)
        // new avg = sum of old corrs + new corr / tau
        // or.. sum of prev corrs = old avg * (tau - 1)
        // so new avg = (sum of prev corrs + new corr) / tau
        // so...
        // new avg = ( old avg * (tau-1) + new corr ) / tau
        // this is faster than re-iterating the average from the list of corrs
        // it is worth noting:
        }
        if(i == 0) {
            cum_avg = corrs[i]; //AFTER the first iteration, the average wil be the first corr
        } else {
            cum_avg = (cum_avg * (i-1) + corrs[-1])/i;
            // this "normalizes" the correlation value (we multiply by 100 because
            // decimal values and integer types don't get along
            corrs[i] = 100 * corrs[i] / cum_avg;
        }

        // IF THE CORRELATION MEETS THE THRESHOLD, SAVE SOME TIME AND EXIT THE LOOP
        if(corrs[i] < correlation_threshhold) {
            free(corrs);
            return ( sample_rate / tau );
        }
    }

    // IF NO FREQUENCY FOUND, RETURN 0
    free(corrs);
    return 0;

}
