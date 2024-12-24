#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
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
int reset_pin_check() {
    return (sio_hw->gpio_in & (1<<RESET_PIN) != 0);
}

#define YIN_WINDOW_WIDTH_MS 10
#define ADC_SAMPLE_RATE_HZ 6000

const uint32_t AUDIO_BUFFER_SIZE = YIN_WINDOW_WIDTH_MS * ADC_SAMPLE_RATE_HZ / 1000;
const uint32_t AUDIO_ADC_CLK_DIV = 48000000 / ADC_SAMPLE_RATE_HZ;

uint16_t audio_buff[AUDIO_BUFFER_SIZE];

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
    uint dma_chan = dma_claim_unused_channel(true);
    printf("DMA Grace Period...\n");
    sleep_ms(1000);


    // Constantly display ADC value (~5ms refresh time)
    multicore_launch_core1(display_ADC);

    int reset_counter = 0;
    while(1) {

        audio_capture_no_blocking(dma_chan, audio_buffer, AUDIO_BUFFER_SIZE);

        sleep_ms(1000);

        //reset check, counter
        sleep_ms(10);
        reset_counter += (sio_hw->gpio_in & (1<<RESET_PIN)) ? 1 : 0;
        if(reset_counter == 100) { reset_usb_boot(0,0); break;}
    }

    return 0;
}



#define ADC_DRAW_INC 5
void display_ADC(void) { // uses a core to constantly print the adc audio buffer to the screen
    i2c_init(i2c1, 400000);

    disp.external_vcc=false;
    ssd1306_init(&disp, SSD1306_WIDTH, SSD1306_HEIGHT, 0x3C, i2c1);
    ssd1306_poweron(&disp);
    ssd1306_clear(&disp);

    while(1) {
        draw_adc_buffer(audio_buff, SSD1306_WIDTH * ADC_DRAW_INC, ADC_DRAW_INC, 4096 / SSD1306_HEIGHT);
        ssd1306_show(&disp);
        sleep_ms(5);
        ssd1306_clear(&disp);
    }
    
    sleep_ms(1000);
    ssd1306_poweroff(&disp);
    sleep_ms(100);
    ssd1306_deinit(&disp);
    
    //}
}
