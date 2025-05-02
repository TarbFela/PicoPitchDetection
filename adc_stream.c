//
// Created by The Tragedy of Darth Wise on 5/2/25.
//

#include "adc_stream.h"


// —————————————————————————————————————————————————————————————————————————————————————————
//                             AUDIO CAPTURE FUNCTION
// —————————————————————————————————————————————————————————————————————————————————————————


//                                +——————————————————————————+
//                                ||         CONSTS         ||
//                                +——————————————————————————+

// The window width has a minimum value equal to
// 2 over the minimum pitch you want to detect, i.e.
//     2 / minimum_detectable_frequency
// Thanks to the behavior of the pitch-detection algorithm
// TODO: An idea which may shorten the minimum window width as much as 50% is this:
//    For very low notes (such as Ab2), instead of looking for a single peak
//    in the correlation function which is less than the threshhold (e.g. 0.1 out of 1),
//    we can look for patterns in the nearby 1st and 2nd harmonics.
//    The algorithm to do this would look for local minima in the correlations
//    and then evaluate them somehow...
//    Realistically, this would work well for notes up to F (tenor sax) (concert Eb)
//    Which has a frequency of about 150Hz, so our window would drop to 13ms.

#define ADC_BASE_CLOCK_HZ             48000000

#define BOTTOM_NOTE 7U // G2
#define NUM_NOTES 36U // G2 to F#5



//                                +——————————————————————————+
//                                ||      AUDIO CAPTURE     ||
//                                +——————————————————————————+

void audio_capture_no_blocking(adc_stream_t *adc_stream) {
    adc_run(false);
    adc_fifo_drain();
    dma_channel_config cfg = dma_channel_get_default_config(adc_stream->dma_channel);
    // Reading from constant address, writing to incrementing byte addresses
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);

    // Pace transfers based on availability of ADC samples
    channel_config_set_dreq(&cfg, DREQ_ADC);

    dma_channel_configure(adc_stream->dma_channel, &cfg,
                          adc_stream->buffer,    // dst
                          &adc_hw->fifo,  // src
                          adc_stream->buffer_size,  // transfer count
                          false            // DON'T start immediately
    );
    adc_run(true);

    dma_channel_start(adc_stream->dma_channel);
}

adc_stream_t *init_adc_stream(uint adc_pin, uint32_t sample_rate_hz, uint32_t n_samples) {

    // ADC Setup modified from pi pico example:
    // https://github.com/raspberrypi/pico-examples/blob/master/adc/dma_capture/dma_capture.c
    adc_init();
    adc_gpio_init(adc_pin);
    adc_select_input(adc_pin - 26);
    adc_fifo_setup(
            true,    // Write each completed conversion to the sample FIFO
            true,    // Enable DMA data request (DREQ)
            1,       // DREQ (and IRQ) asserted when at least 1 sample present
            false,   // We won't see the ERR bit because of 8 bit reads; disable.
            false     // Shift each sample to 8 bits when pushing to FIFO?
    );
    // Divisor of 0 -> full speed. Free-running capture with the divider is
    // equivalent to pressing the ADC_CS_START_ONCE button once per `div + 1`
    // cycles (div not necessarily an integer). Each conversion takes 96
    // cycles, so in general you want a divider of 0 (hold down the button
    // continuously) or > 95 (take samples less frequently than 96 cycle
    // intervals). This is all timed by the 48 MHz ADC clock.

    // divisor of  8000 -> 48MHz / 8000 = 6kHz (currently 20kHz...)
    adc_set_clkdiv(ADC_BASE_CLOCK_HZ / sample_rate_hz);

    adc_stream_t *s;
    s = (adc_stream_t *)malloc(sizeof(adc_stream_t));
    s->adc_sample_rate = sample_rate_hz;
    s->buffer_size = n_samples;
    s->dma_channel = dma_claim_unused_channel(true);
    s->buffer = (uint16_t *)malloc(sizeof(uint16_t) * n_samples);
    return s;
}


