/*
 * panWebcam.ino
 *
 * Tom Price 29 Sep 2012
 * uses Guitar Hero footpedal to pan a servo mounted webcam
 * 
 * http://smokedprojects.blogspot.com/2012/09/arduino-foot-pedal.html
 * http://smokedprojects.blogspot.com/2012/09/hands-free-panning-webcam.html
 */

#include <PS2X_lib.h>  //for v1.6
#include <PWMServo.h>

/* NB PWMServo is the old arduino library from version 16
 *    distributed at http://arduiniana.org/libraries/pwmservo/
 *
 * for Arduino 1.0 and later:
 *
 * need to comment out #include <wiring.h> in PWMServo.cpp
 * and substitute this:
 *
#if ARDUINO > 22
#include "Arduino.h"
#else
#include <wiring.h>
#endif
#include <PWMServo.h>
 *
 */

PS2X ps2x; // create PS2 Controller Class

PWMServo servo; // servo that pans webcam

//right now, the library does NOT support hot pluggable controllers, meaning 
//you must always either restart your Arduino after you conect the controller, 
//or call config_gamepad(pins) again after connecting the controller.
int error = 0; 
byte type = 0;
byte vibrate = 0;
int pos = 0x80;

void setup()
{
  servo.attach(9); // attach servo to Arduino pin 9
  
  Serial.begin(57600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  //CHANGES for v1.6 HERE!!! **************PAY ATTENTION*************
  error = ps2x.config_gamepad();   //setup pins and settings:  GamePad(clock, command, attention, data) check for error
  Serial.print("Error:");
  Serial.println(error,DEC);

  if (error == 0)
  {
    Serial.println("Found Controller, configured successful");
    Serial.println("Try out all the buttons, X will vibrate the controller, faster as you press harder;");
    Serial.println("holding L1 or R1 will print out the analog stick values.");
    Serial.println("Go to www.billporter.info for updates and to report bugs.");
  }
  else if (error == 1)
  {
    Serial.println("No controller found, check wiring, see readme.txt to enable debug. visit www.billporter.info for troubleshooting tips");
  }
  else if (error == 2)
  {
    Serial.println("Controller found but not accepting commands. see readme.txt to enable debug. Visit www.billporter.info for troubleshooting tips");
  }
  else if (error == 3)
  {
    Serial.println("Controller refusing to enter Pressures mode, may not support it. ");
  }
  else
  {
    Serial.println("Unknown controller error code");
  }
  //Serial.print(ps2x.Analog(1), HEX);

  type = ps2x.readType(); 
  Serial.print("Type:");
  Serial.println(type,DEC);
  
  switch(type) {
  case 1:
    Serial.println("DualShock Controller Found");
    break;
  case 2:
    Serial.println("GuitarHero Controller Found");
    break;
  case 0:
  default:
    Serial.println("Unknown Controller type");
    break;
  }
}

void loop()
{
  /* You must Read Gamepad to get new values
   Read GamePad and set vibration values
   ps2x.read_gamepad(small motor on/off, larger motor strenght from 0-255)
   if you don't enable the rumble, use ps2x.read_gamepad(); with no values
   
   you should call this at least once a second
   */

  if (error == 1) // skip loop if no controller found
    return; 

  if (type == 2) // Guitar Hero Controller
  { 

    ps2x.read_gamepad();          // read controller 

    Serial.print("Star Power:");
    Serial.println(ps2x.Button(STAR_POWER), HEX);
    Serial.print("Whammy Bar Position:");
    pos = ps2x.Analog(WHAMMY_BAR);
    Serial.println(pos, HEX);
    
    // position servo
    Serial.print("Servo Position:");
    pos = map(pos, 0, 128, 120, 60); // 60 degree pan
    Serial.println(pos, DEC);
    servo.write(pos);
  }
  delay(15);
}


