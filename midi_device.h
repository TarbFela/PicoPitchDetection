//
// Created by The Tragedy of Darth Wise on 12/30/24.
//

#ifndef MIDI_DEVICE_H
#define MIDI_DEVICE_H

#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"

//#include "bsp/board.h"
#include "tusb.h"



/*

//∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆\\
\\V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V//


                      CONTROL LOGIC DOCUMENTATION
       The point of my device is to detect saxophone pitches and turn them into
    MIDI notes.

       As such, the main.c code should focus on detecting pitches, and then this
    midi_device should handle the whole state-machine
       Since the RP2040 on the pico is capable of multitasking with 2 cores, I'd
    also like to design this module with that in mind. Again, the less control
    code there is in main.c for midi, the better. As such, the control logic
    should be handled be a more general wrapper function here, which I'll call
               "midi_control()"
     This function will take make this module act as if it were a separate
    device that takes commands and does what it wants to with them. The commands
    will, at present, just be integer-value pitches from the pitch detection
    function, or zeroes for a no-pitch-detected. This is exactly the output
    behavior of the find_frequency() function in pitch_analysis.c

     The midi_control() function should be called repeatedly in a loop. This
    makes it easier to debug.

     The actual control logic should be roughly as follows:
        If a note is detected:
          Start a new note
          End the current note
          (this order makes it easier for VSTs with glide to actually glide.
          It might be a good idea to make it an option to wait to cut the old
          note off)
        If no note is detected:
          Wait for enough no-notes to be registered before ending the
          current note

     Some fun ideas for different behaviors:
        1) Auto-chords
        2) Arpeggios & "delay"
          -> Use the timing scheme from the pico-example-midi.c midi_task()
             function
        3) Note(s) sustain
          -> Essentially, wait to turn the previous note(s) off until a certain
             number of new notes and/or a certain time amount

//∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆\\
\\V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V∆V//

 */

void midi_init();

uint32_t midi_control(int ctrl_message);

int midi_note_on(uint8_t pitch);

int midi_note_off(uint8_t pitch);

void midi_message(uint8_t msg_type, uint8_t msg_data_0, uint8_t msg_data_1);


#endif //MIDI_DEVICE_H
