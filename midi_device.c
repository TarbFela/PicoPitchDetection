//
// Code adapted and copied from https://github.com/infovore/pico-example-midi
//
#include "midi_device.h"

#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"

//#include "bsp/board.h"
#include "tusb.h"

//--------------------------------------------------------------------+
// Device callbacks (set to do nothing currently)
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
  return;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
  return;
}

//  Probably not useful for my application...
// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void) remote_wakeup_en;
  return;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
  return;
}

//--------------------------------------------------------------------+
// MIDI Task
//--------------------------------------------------------------------




void midi_init() {
  tusb_init();
}

#define NO_NOTE_TIMEOUT_THRESH 10
#define BOTTOM_MIDI_NOTE 1 // though there is a midi 0 pitch, our control logic is (foolishly) reserving that for other purposes
#define TOP_MIDI_NOTE 127

uint32_t midi_control(int ctrl_message) {
  // variables which must be remembered!
  static uint8_t current_on_note = 0;
  static uint32_t no_note_timeout_counter = 0;

  tud_task();

  if(ctrl_message == 0) { // no note
    // increment counter, check against threshhold and verify that there is a note being played in the first place
    if(no_note_timeout_counter++ > NO_NOTE_TIMEOUT_THRESH && current_on_note != 0 ) {
      midi_note_off(current_on_note);
      current_on_note = 0;
     }
  }
  else {
    no_note_timeout_counter = 0;
  }

  // could I have this all be one big if-else? yes. would that save time? a tiny bit. however, this way I can be sure of every behavior, hopefully!
  // if the ctrl_message is a pitch in the range of acceptable midi notes, send the new note, end the old one.
  if( BOTTOM_MIDI_NOTE <= ctrl_message && ctrl_message <= TOP_MIDI_NOTE) {
    if( ctrl_message == current_on_note) continue; //if we're already playing this note, don't do anything
    midi_note_on(ctrl_message);
    midi_note_off(current_on_note);
    current_on_note = (uint8_t)ctrl_message; // explicit cast! take that, compiler warning!
  }

  return
}


// midi_note_on() and off() both return -1 for bad inputs.

#define MIDI_NOTE_ON 0x90
#define MIDI_NOTE_OFF 0x80
#define MIDI_NOTE_DEFAUlT_VELOCITY 64

int midi_note_on(uint8_t pitch) {
  // can't hurt to double-check!
  if( BOTTOM_MIDI_NOTE > ctrl_message || ctrl_message > TOP_MIDI_NOTE) return -1;
  midi_message( MIDI_NOTE_ON, pitch, MIDI_NOTE_DEFAULT_VELOCITY);
  return 0;
}

int midi_note_off(uint8_t pitch) {
  if( BOTTOM_MIDI_NOTE > ctrl_message || ctrl_message > TOP_MIDI_NOTE) return -1;
  midi_message( MIDI_NOTE_OFF, pitch, MIDI_NOTE_DEFAULT_VELOCITY);
  return 0;
}

void midi_message(uint8_t msg_type, uint8_t msg_data_0, uint8_t msg_data_1)
{
  uint8_t msg[3] = {msg_type, msg_data_0, msg_data_1};
  tud_midi_n_stream_write(0, 0, msg, 3);
}


