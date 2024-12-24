# Raspberry Pi Pico Real Time Pitch Detection

## Work in Progress

For live pitch detection. Takes analog input from the ADC (channel 0, GPIO 26), and puts it into a uint16_t buffer using the DMA.

There are several optimization and specification choices:

* The sample rate of the ADC is 6kHz. This is because the highest pitch I want to detect is somewhere below E7
* The size of the audio buffer is 10ms x 6kHz x 2 = 120 samples. This is because the lowest pitch I want to detect is Ab2 (103Hz --> 10ms)

Currently, this is in the development stage. I am using an SSD1306 display to debug, i.e. to act as an oscilliscope for the ADC data. I have yet to test any of this code.
