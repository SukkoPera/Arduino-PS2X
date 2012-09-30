/*
 * Tom Price 29 Sep 2012
 * http://smokedprojects.blogspot.com/2012/09/arduino-foot-pedal.html
 *
 * Guitar hero emulator
 * based on fork of Bill Porter's PS2X library
 * http://www.billporter.info/?p=240
 */

#include <PS2X_lib.h>

void setup()
{
  Serial.begin(57600);
}

void loop()
{
  PS2X guitar;
  guitar.emulateGuitar();
}
