# Raspberry Pi Pico Real Time Pitch Detection

## Work in Progress

### Hypothesis for testing

**Issue:** the pitch detection yields a low confidence, since the Yin Correlation curve is noisy. 
Instead of trying to manhandle the YCC, I can instead focus _only_ on the pitches in which I am interested.
At the same time, we can acheive greater granularity by increasing the sample rate.\

So:
1. Search through specified indices of the YCC for the lowest-frequency minimum.
2. Increase the sample rate (?)

### Overview

For live pitch detection. Takes analog input from the ADC, and puts it into a uint16_t buffer using the DMA.

(**_Once debugging is done_** there will be) several optimization and specification choices:

* The sample rate of the ADC is 6kHz. This is because the highest pitch I want to detect is somewhere below E7
* The size of the audio buffer is 10ms x 6kHz x 2 = 120 samples. This is because the lowest pitch I want to detect is Ab2 (103Hz --> 10ms)

Currently, this is in development

### Debugging

Currently, I am working to improve the pitch detection algorithm. The primary issue I am running into is the noise floor of the Pi Pico's ADC. This is a hardware issue with a hardware solution.

My debug environment is set up in usb_debug.c.

- The pico waits for stdio input (a "q" will cause a usb-reboot)
- The python script, live_output_v2.py, finds the correct usb modem (on **Mac**) and then starts pinging the pico with inputs and plotting the output.
- The pico prints out arrays of the input buffer as well as the pitch detection algorithm period-correlation buffer.
- If the python program is interrupted by SIGINT (keyboard interrupt, etc) the pico will continue to wait for input.
- If the python program is ended by closing the pyplot window, the pico will reboot to usb and can be flashed again.

### Hardware Considerations, Circuitry

The accuracy of the outputs depends on the quality of the inputs. 
A loud, clean signal with low noise will be easier to accurately analyze 
than a quiet signal. For this reason, we need some gain between the line-level input and the ADC.

#### Amplifier Circuit

Currently, I am using and ALD1105 quad-MOSFET package with VSS = 5V (from the V_BUS pico pin) and VDD = 0V. It has 2 NMOS and 2 PMOS transistors. 
I am using the PMOS in a current-mirror configuration with an R_REF of ~20kΩ (±5%). This is biasing a 
common-source NMOS gain stage with a source degeneration resistor of about 8.8kΩ (±10%). The input is provided to the gate
via a resistive biasing network of 22kΩ R1 and about 25kΩ R2 (adjusted experimentally to get the signal operating in the linear region).
The gate D.C. voltage is roughly 2.7V, and a decoupling capacitor in the ~1µF range was used to keep the cutoff frequency well below 100Hz.

I'd like to add another gain stage, or to simply increase the gain of the first stage. If I do this, I ought to add a volume control 
so that I can attenuate the input if there is significant distortion or clipping. 

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


