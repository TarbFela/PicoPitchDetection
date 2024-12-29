#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "pico/multicore.h"
#include "pico/bootrom.h"

#include "ssd1306.h"
#include "BMSPA_font.h"

#define SSD1306_HEIGHT      64
#define SSD1306_WIDTH       128
#define SSD1306_CENTER_X    64
#define SSD1306_CENTER_Y    32
#define DISPLAY_LEFT_MARGIN 5
#define ROW_HEIGHT 8

/*
ssd1306_t disp;
void disp_init() {
    disp.external_vcc=false;
    ssd1306_init(&disp, SSD1306_WIDTH, SSD1306_HEIGHT, 0x3C, i2c1);
    ssd1306_poweron(&disp);
    ssd1306_clear(&disp);
}

void display_ADC(void);

void display_text(int row, const char *s) {
    int x = DISPLAY_LEFT_MARGIN;
    int y = row*ROW_HEIGHT;
    ssd1306_draw_string(&disp, x, y, 1, s);
}

void display_adc_value(int row, int channel) {
    adc_select_input(channel);
    uint32_t value = adc_read();
    static char buf[20];
    sprintf(buf, "%d, 0x%X", value, value);
    display_text(row, buf);
}

// draw from a buffer of values accross the screen.
// Will overlap onto itself vertically and horizontally.
// Takes unsigned values, so bottom of screen = 0
// Will access every "increment" index of the buff according to the size
void draw_adc_buffer(uint16_t *adc_buff, int buff_size, int increment, int height_div) {
    for(int i = 0; i< buff_size; i+= increment) {
        ssd1306_draw_pixel( &disp, i % SSD1306_WIDTH, (adc_buff[i] / height_div) % SSD1306_HEIGHT);
    }
}

void display_number(int row, int number) {
    static char buf[20];
    sprintf(buf, "%d", number);
    display_text(row, buf);
}

 */


#define RESET_PIN 1
int reset_pin_check() {
    return (sio_hw->gpio_in & (1<<RESET_PIN) != 0);
}

#define YIN_WINDOW_WIDTH_MS 20
#define ADC_SAMPLE_RATE_HZ 20000
#define ADC_INPUT_PIN 26



#define AUDIO_BUFFER_SIZE          (YIN_WINDOW_WIDTH_MS * ADC_SAMPLE_RATE_HZ / 1000)
#define AUDIO_BUFFER_SIZE_HALF     (AUDIO_BUFFER_SIZE/2)
#define AUDIO_ADC_CLK_DIV          (48000000 / ADC_SAMPLE_RATE_HZ)

uint16_t audio_buffs[2][AUDIO_BUFFER_SIZE];


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

// function to take a buffer of uint16_t and return the calculated pitch in Hz (as a float)
// Will return -1 if there's an issue


float detect_pitch(uint16_t *buff) {
    float* dp_diff_buff = NULL;
    dp_diff_buff = (float*) malloc( AUDIO_BUFFER_SIZE_HALF * sizeof(float));
    if(dp_diff_buff == NULL) return -1;


    // for every offset ( o samples ), check the correlation:
    // sum[ ( audio[t] - audio[o] ) ^ 2 ]
    for( int o = 1; o < AUDIO_BUFFER_SIZE / 2; o++) {
    //note: iterate through half the buffer
    // because the offset can be as high as half the buffer, so max(t + o) = buffer size
        for( int t = 0; t < AUDIO_BUFFER_SIZE / 2; o++ ) {
            int diff = ( buff[t] - buff[t + o] );
            dp_diff_buff[o] += diff * diff;  // could replace with an int array and reduce compute time significantly if required
        }
    }

    float max_corr = 0;
    int max_corr_index = -1;
    for( int o = 0; o < AUDIO_BUFFER_SIZE / 2; o++) {
        if(max_corr < dp_diff_buff[o]) {
            max_corr = dp_diff_buff[o];
            max_corr_index = o;
        }
    }

    free(dp_diff_buff);

    if (max_corr_index == -1) return -1; //if something went wrong, return -1
    // the max_corr_index corresponds to an o-sample period.
    // The corresponding frequency is:
    // sample rate / o
    return ADC_SAMPLE_RATE_HZ / max_corr_index;
}

uint dma_chan; // globally accessible! May pose an issue for syntax...
int main() {
    stdio_init_all();



    // RESET PIN
    gpio_init(RESET_PIN);
    gpio_pull_down(RESET_PIN);


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
        true     // Shift each sample to 8 bits when pushing to FIFO
    );
    // Divisor of 0 -> full speed. Free-running capture with the divider is
    // equivalent to pressing the ADC_CS_START_ONCE button once per `div + 1`
    // cycles (div not necessarily an integer). Each conversion takes 96
    // cycles, so in general you want a divider of 0 (hold down the button
    // continuously) or > 95 (take samples less frequently than 96 cycle
    // intervals). This is all timed by the 48 MHz ADC clock.

    // divisor of  8000 -> 48MHz / 8000 = 6kHz
    adc_set_clkdiv(AUDIO_ADC_CLK_DIV);

    printf("ADC Grace Period...\n");
    sleep_ms(1000);
    // Set up the DMA to start transferring data as soon as it appears in FIFO
    dma_chan = dma_claim_unused_channel(true);
    printf("DMA Grace Period...\n");
    sleep_ms(1000);


    // Constantly display ADC value (~5ms refresh time)
    //multicore_launch_core1(display_ADC);

    int reset_counter = 0;
    uint8_t abrr_i = 0; // audio buffer round robin
                        // useful for the case where we want to take in audio,
                        // and then continue to take in audio while processing the previous batch
    char user_input = 0;

    while( user_input != 'q' ) {

        for(int ii = 0; ii < 100; ii ++) {

            //abrr_i = ( abrr_i + 1 ) % 2;
            audio_capture_no_blocking(dma_chan, audio_buffs[0], AUDIO_BUFFER_SIZE);

            dma_channel_wait_for_finish_blocking(dma_chan);
            //printf("Audio buffer:");
            for(int i = 0; i<AUDIO_BUFFER_SIZE; i++) printf("%d,", audio_buffs[0][i]);
            printf("\n");
            //reset check, counter
            sleep_ms(10);
            //reset_counter += (sio_hw->gpio_in & (1<<RESET_PIN)) ? 1 : 0;
            //if(reset_counter == 100) { reset_usb_boot(0,0); break;}
        }
        scanf("%c", &user_input);
        printf("\n\n\t\t%c\n",user_input);
        sleep_ms(1000);
    }

    reset_usb_boot(0,0);
    return 0;
}


/*
#define ADC_DRAW_INC 5
void display_ADC(void) { // uses a core to constantly print the adc audio buffer to the screen
    i2c_init(i2c1, 400000);

    disp.external_vcc=false;
    ssd1306_init(&disp, SSD1306_WIDTH, SSD1306_HEIGHT, 0x3C, i2c1);
    ssd1306_poweron(&disp);
    ssd1306_clear(&disp);

    while(1) {
        ssd1306_clear(&disp);
        dma_channel_wait_for_finish_blocking(dma_chan);
        // can replace the following line with a display_text of a sprintf of the detected pitch from main()
        draw_adc_buffer(audio_buffs[0], SSD1306_WIDTH * ADC_DRAW_INC, ADC_DRAW_INC, 4096 / SSD1306_HEIGHT);
        ssd1306_show(&disp);
    }
    
    sleep_ms(1000);
    ssd1306_poweroff(&disp);
    sleep_ms(100);
    ssd1306_deinit(&disp);
    
    //}
}

 */
