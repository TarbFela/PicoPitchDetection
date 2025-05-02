//
// Created by The Tragedy of Darth Wise on 5/2/25.
//

#ifndef PITCHDETECTION_ADC_STREAM_H
#define PITCHDETECTION_ADC_STREAM_H


#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    uint dma_channel;
    uint16_t *buffer;
    uint32_t buffer_size;
    uint32_t adc_sample_rate;
} adc_stream_t;

adc_stream_t *init_adc_stream(uint adc_pin, uint32_t sample_rate_hz, uint32_t n_samples);

// kicks off dma to start populating the audio buffer
void audio_capture_no_blocking(adc_stream_t *adc_stream);


#endif //PITCHDETECTION_ADC_STREAM_H
