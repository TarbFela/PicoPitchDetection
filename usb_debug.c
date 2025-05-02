#define true 1
#define false 0


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
//#include <string.h>

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "pico/multicore.h"
#include "pico/bootrom.h"

#include "pitch_analysis.h"

#define RESET_PIN 1
int reset_pin_check() {
    return ((sio_hw->gpio_in & (1<<RESET_PIN)) != 0);
}


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

#define YIN_WINDOW_WIDTH_MS 20
#define ADC_SAMPLE_RATE_HZ 15000
#define ADC_INPUT_PIN 28


#define AUDIO_BUFFER_SIZE          (YIN_WINDOW_WIDTH_MS * ADC_SAMPLE_RATE_HZ / 1000)
#define AUDIO_BUFFER_SIZE_HALF     (AUDIO_BUFFER_SIZE/2)
#define AUDIO_ADC_CLK_DIV          (48000000 / ADC_SAMPLE_RATE_HZ)

#define N_AUDIO_SUBBUFFERS          4
#define AUDIO_SUBBUFFER_SIZE        AUDIO_BUFFER_SIZE / N_AUDIO_SUBBUFFERS

#define BOTTOM_NOTE 7U // G2
#define NUM_NOTES 36U // G2 to F#5


//                                +——————————————————————————+
//                                ||      AUDIO BUFFER      ||
//                                +——————————————————————————+

uint16_t audio_buffs[2][AUDIO_BUFFER_SIZE];


//                                +——————————————————————————+
//                                ||      AUDIO CAPTURE     ||
//                                +——————————————————————————+

void audio_capture_no_blocking(uint dma_chan, uint16_t *buff, size_t buff_size) {
    adc_run(false);
    adc_fifo_drain();
    dma_channel_config cfg = dma_channel_get_default_config(dma_chan);
    // Reading from constant address, writing to incrementing byte addresses
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);

    // Pace transfers based on availability of ADC samples
    channel_config_set_dreq(&cfg, DREQ_ADC);

    dma_channel_configure(dma_chan, &cfg,
        buff,    // dst
        &adc_hw->fifo,  // src
        AUDIO_BUFFER_SIZE,  // transfer count
        false            // DON'T start immediately
    );
    adc_run(true);

    dma_channel_start(dma_chan);
}


// —————————————————————————————————————————————————————————————————————————————————————————
//                                         MULTICORE CODE
// —————————————————————————————————————————————————————————————————————————————————————————

#define MULTICORE_GOOD_FLAG 0xBEEF

void core1_entry() {

    // initialize...
    multicore_fifo_push_blocking(MULTICORE_GOOD_FLAG); // tell main that we're good

    FREQ_ANALYZER_T *fa_s = (FREQ_ANALYZER_T *)multicore_fifo_pop_blocking(); // get access to the freq analyzer struct

    // an array of 24 pitches (G2 98Hz to F#4 370) expressed at sample-offsets in increasing order in terms of tau
    uint32_t test_tau_array[NUM_NOTES];
    for(uint32_t tt = NUM_NOTES + BOTTOM_NOTE - 1; tt >= BOTTOM_NOTE && tt < NUM_NOTES + BOTTOM_NOTE; tt--) {
        uint32_t pitchfreq = PITCH_FREQS[(tt%12)]; // get Oct2 pitch
        for(int oct = 0; oct < (tt/12); oct++) {
            pitchfreq *= 2;
        }
        test_tau_array[tt-BOTTOM_NOTE] = fa_s->sample_rate / pitchfreq;
    }


    int pitch = -1;
    int frequency, volume = 0;
    int i = 0;
    while(1) {
        multicore_fifo_pop_blocking();

        volume = get_peak_volume(fa_s);

        calculate_yin(fa_s);
        if(volume > 25) {
            frequency = dominant_freq(fa_s);
        }
        else {
            frequency = 0;
        }


        //print out input buffer
        printf("\nINPUT BUFFER: ");
        for( int sample = 0; sample < AUDIO_BUFFER_SIZE; sample++ ) {
            printf("%d, ",fa_s->audio_buffer[sample]);
        }
        //print out corrs buffer
        printf("\nCORRS BUFFER: ");
        for( int tau = 0; tau < fa_s->corrs_arr_size; tau++) printf("%d, ", fa_s->correlations_array[tau]);
        printf("\nTAU VALUE: %d", frequency);
        printf(" EOT\n"); // signal end of transmission
        multicore_fifo_push_blocking(1); // tell main to continue


        i += 1;
    }


}


uint dma_chan; // globally accessible! May pose an issue for syntax...
int main() {

    // —————————————————————————————————————————————————————————————————————————————————————————
    //                                         INITS
    // —————————————————————————————————————————————————————————————————————————————————————————

    stdio_init_all();
    // RESET PIN
    gpio_init(RESET_PIN);
    gpio_pull_down(RESET_PIN);


//                                +——————————————————————————+
//                                ||         ADC INIT       ||
//                                +——————————————————————————+


    // TODO: consolidate adc and dma inits and controls into separate file
  // ADC Setup modified from pi pico example:
  // https://github.com/raspberrypi/pico-examples/blob/master/adc/dma_capture/dma_capture.c
    adc_init();
    adc_gpio_init(ADC_INPUT_PIN);
    adc_select_input(ADC_INPUT_PIN - 26);
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
    adc_set_clkdiv(AUDIO_ADC_CLK_DIV);

    printf("ADC Grace Period...\n");
    sleep_ms(1000);


//                                +——————————————————————————+
//                                ||        DMA INIT        ||
//                                +——————————————————————————+

    // Set up the DMA to start transferring data as soon as it appears in FIFO
    dma_chan = dma_claim_unused_channel(true);
    printf("DMA Grace Period...\n");
    sleep_ms(3000);

//                                +——————————————————————————+
//                                ||     MULTICORE INIT     ||
//                                +——————————————————————————+

    multicore_launch_core1(core1_entry);
    uint32_t mcfifo_val = multicore_fifo_pop_blocking();
    if(mcfifo_val != MULTICORE_GOOD_FLAG) printf("Failed to initialize core 1.\n");

//                                +——————————————————————————+
//                                ||   FREQ ANALYZER INIT   ||
//                                +——————————————————————————+
    FREQ_ANALYZER_T *fa_s;
    fa_s = init_freq_analyzer(AUDIO_BUFFER_SIZE, ADC_SAMPLE_RATE_HZ, 50);
    multicore_fifo_push_blocking((uint32_t)fa_s);

    sleep_ms(1000);

    // —————————————————————————————————————————————————————————————————————————————————————————
    //                                      MAIN LOOP
    // —————————————————————————————————————————————————————————————————————————————————————————

    char ui[64];
    for(int i = 0; i<63; i++) ui[i] = '.';
    ui[63] = 0;


    while(1) {
        scanf("%s",&ui);
        if(ui[0] == 'q') break;
        audio_capture_no_blocking(dma_chan, fa_s->audio_buffer, AUDIO_BUFFER_SIZE); //start capture
        dma_channel_wait_for_finish_blocking(dma_chan); //wait for end of capture
        multicore_fifo_push_blocking(0); //start printout
        multicore_fifo_pop_blocking(); //wait for end of printout
    }


    reset_usb_boot(0,0);
    return 0;
}
