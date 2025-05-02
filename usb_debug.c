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

#include "adc_stream.h"

#define ADC_GPIO_PIN 27 // pin 27, adc_1
#define RESET_PIN 1
int reset_pin_check() {
    return ((sio_hw->gpio_in & (1<<RESET_PIN)) != 0);
}

// 20ms lets us see (theoretically) 50Hz
// 15kHz gives us enough granularity to differentiate higher pitches
#define YIN_WINDOW_WIDTH_MS 20 // how many ms of audio do we want to analyze? See Nyquist freq sampling.
#define ADC_SAMPLE_RATE_HZ 15000 // what's our sample rate? See Nyquist time sampling
#define AUDIO_BUFFER_SIZE          (YIN_WINDOW_WIDTH_MS * ADC_SAMPLE_RATE_HZ / 1000)

#define MULTICORE_GOOD_FLAG 0xBEEF
void core1_entry() {

    // initialize...
    multicore_fifo_push_blocking(MULTICORE_GOOD_FLAG); // tell main that we're good

    FREQ_ANALYZER_T *fa_s = (FREQ_ANALYZER_T *)multicore_fifo_pop_blocking(); // get access to the freq analyzer struct

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

int main() {

    // —————————————————————————————————————————————————————————————————————————————————————————//
    //                                         INITS                                            //
    // —————————————————————————————————————————————————————————————————————————————————————————//

    stdio_init_all();
    // RESET PIN
    gpio_init(RESET_PIN);
    gpio_pull_down(RESET_PIN);


//                                +——————————————————————————+
//                                ||         ADC INIT       ||
//                                +——————————————————————————+

    adc_stream_t *adc_stream = init_adc_stream(ADC_GPIO_PIN, ADC_SAMPLE_RATE_HZ, AUDIO_BUFFER_SIZE);
    sleep_ms(1000);

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
    fa_s = init_freq_analyzer(adc_stream->buffer, adc_stream->buffer_size, adc_stream->adc_sample_rate, 50);
    multicore_fifo_push_blocking((uint32_t)fa_s);
    sleep_ms(1000);

    // —————————————————————————————————————————————————————————————————————————————————————————//
    //                                      MAIN LOOP                                           //
    // —————————————————————————————————————————————————————————————————————————————————————————//

    char ui[64];
    for(int i = 0; i<63; i++) ui[i] = '.';
    ui[63] = 0;

    while(1) {
        scanf("%s",&ui);
        if(ui[0] == 'q') break;
        audio_capture_no_blocking(adc_stream); //start capture
        dma_channel_wait_for_finish_blocking(adc_stream->dma_channel); //wait for end of capture
        multicore_fifo_push_blocking(0); //start printout
        multicore_fifo_pop_blocking(); //wait for end of printout
    }

    reset_usb_boot(0,0);
    return 0;
}