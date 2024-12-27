# Raspberry Pi Pico Real Time Pitch Detection

## Work in Progress

For live pitch detection. Takes analog input from the ADC (channel 0, GPIO 26), and puts it into a uint16_t buffer using the DMA.

There are several optimization and specification choices:

* The sample rate of the ADC is 6kHz. This is because the highest pitch I want to detect is somewhere below E7
* The size of the audio buffer is 10ms x 6kHz x 2 = 120 samples. This is because the lowest pitch I want to detect is Ab2 (103Hz --> 10ms)

Currently, this is in development

## Current Use Instructions:
I am working using a Macbook.

The program is currently set up for easy debugging.

1) Compile and upload
   1) ensure that the pico is boot mode
    >make

    > cp Pitch_Detection.uf2 /Volumes/RPI-RP2

2) Find out what modem the serial connection is on

    > ls /dev/ | grep cu.usbmodem

3) You can see the raw output using screen (press any key to start a capture, and 'q' to return to boot)
    
    >screen /dev/cu.usbmodem**XXXX** 115200

4) Alternatively (and more effectively) you can save the audio captures:

    >    echo ' ' > /dev/cu.usbmodem**XXXX**; cat /dev/cu.usbmodem**XXXX** > ex_output.txt

5) You can send the pico to boot mode after the first capture or any subsequent capture by echoing 'q' into the usb modem 

7) If you have the data, you can analyze more easily, using tools such as python (see the ex_output_XXX.py) files


