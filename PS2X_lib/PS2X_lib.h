/*
 * Tom Price's fork of the Arduino PS2X library 29 Sept 2012
 * http://smokedprojects.blogspot.com/2012/09/arduino-foot-pedal.html 
 * 
 * Changelog:
 *
 * o  Pin definitions changed from Arduino PinMode to AVR PORT/PIN
 *
 * o  Some extra bits added to the serial handshaking protocol
 *    Most saliently control of the ACK line
 *
 * o  A new method PS2X::emulateGuitar() allowing the Arduino
 *    to emulate a Guitar Hero controller by handling the
 *    controller side of the serial communication protocol
 *
 * NB in my hands the timing of the bitbanged serial communication
 *    only works with DEBUG mode enabled
 */


/******************************************************************
*  Super amazing PS2 controller Arduino Library v1.8
*		details and example sketch: 
*			http://www.billporter.info/?p=240
*
*    Original code by Shutter on Arduino Forums
*
*    Revamped, made into lib by and supporting continued development:
*              Bill Porter
*              www.billporter.info
*
*	 Contributers:
*		Eric Wetzel (thewetzel@gmail.com)
*		Kurt Eckhardt
*
*  Lib version history
*    0.1 made into library, added analog stick support. 
*    0.2 fixed config_gamepad miss-spelling
*        added new functions:
*          NewButtonState();
*          NewButtonState(unsigned int);
*          ButtonPressed(unsigned int);
*          ButtonReleased(unsigned int);
*        removed 'PS' from begining of ever function
*    1.0 found and fixed bug that wasn't configuring controller
*        added ability to define pins
*        added time checking to reconfigure controller if not polled enough
*        Analog sticks and pressures all through 'ps2x.Analog()' function
*        added:
*          enableRumble();
*          enablePressures();
*    1.1  
*        added some debug stuff for end user. Reports if no controller found
*        added auto-increasing sentence delay to see if it helps compatibility.
*    1.2
*        found bad math by Shutter for original clock. Was running at 50kHz, not the required 500kHz. 
*        fixed some of the debug reporting. 
*	1.3 
*	    Changed clock back to 50kHz. CuriousInventor says it's suppose to be 500kHz, but doesn't seem to work for everybody. 
*	1.4
*		Removed redundant functions.
*		Fixed mode check to include two other possible modes the controller could be in.
*       Added debug code enabled by compiler directives. See below to enable debug mode.
*		Added button definitions for shapes as well as colors.
*	1.41
*		Some simple bug fixes
*		Added Keywords.txt file
*	1.5
*		Added proper Guitar Hero compatibility
*		Fixed issue with DEBUG mode, had to send serial at once instead of in bits
*	1.6
*		Changed config_gamepad() call to include rumble and pressures options
*			This was to fix controllers that will only go into config mode once
*			Old methods should still work for backwards compatibility 
*    1.7
*		Integrated Kurt's fixes for the interrupts messing with servo signals
*		Reorganized directory so examples show up in Arduino IDE menu
*    1.8
*		Added Arduino 1.0 compatibility. 
*
*
*
*This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
<http://www.gnu.org/licenses/>
*  
******************************************************************/

/* Modified version for Guitar Hero pedals
 * August 2012
 */


/* $$$$$$$$$$$$ DEBUG ENABLE SECTION $$$$$$$$$$$$$$$$
 * these 2 lines uncommented to print out debug to uart
 *
 * Tom Price says:
 *
 * NB in my hands the timing of the bitbanged serial communication
 *    only works with DEBUG mode enabled
 */

#define PS2X_DEBUG
#define PS2X_COM_DEBUG


#ifndef PS2X_lib_h
#define PS2X_lib_h
#if ARDUINO > 22
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>

#define CTRL_CLK        4
#define CTRL_BYTE_DELAY 3

//These are our button constants
#define PSB_SELECT      0x0001
#define PSB_L3          0x0002
#define PSB_R3          0x0004
#define PSB_START       0x0008
#define PSB_PAD_UP      0x0010
#define PSB_PAD_RIGHT   0x0020
#define PSB_PAD_DOWN    0x0040
#define PSB_PAD_LEFT    0x0080
#define PSB_L2          0x0100
#define PSB_R2          0x0200
#define PSB_L1          0x0400
#define PSB_R1          0x0800
#define PSB_GREEN       0x1000
#define PSB_RED         0x2000
#define PSB_BLUE        0x4000
#define PSB_PINK        0x8000
#define PSB_TRIANGLE    0x1000
#define PSB_CIRCLE      0x2000
#define PSB_CROSS       0x4000
#define PSB_SQUARE      0x8000

//Guitar  button constants
#define GREEN_FRET		0x0200
#define RED_FRET		0x2000
#define YELLOW_FRET		0x1000
#define BLUE_FRET		0x4000
#define ORANGE_FRET		0x8000
#define STAR_POWER		0x0100
#define UP_STRUM		0x0010
#define DOWN_STRUM		0x0040
#define WHAMMY_BAR		8

//These are stick values
#define PSS_RX 5
#define PSS_RY 6
#define PSS_LX 7
#define PSS_LY 8

//PS2 controller modes
#define MODE_DIGITAL 	0x41
#define MODE_ANALOGUE	0x73
#define MODE_CONFIG  	0xF3

//These are analog buttons
#define PSAB_PAD_RIGHT   9
#define PSAB_PAD_UP      11
#define PSAB_PAD_DOWN    12
#define PSAB_PAD_LEFT    10
#define PSAB_L2          19
#define PSAB_R2          20
#define PSAB_L1          17
#define PSAB_R1          18
#define PSAB_GREEN       13
#define PSAB_RED         14
#define PSAB_BLUE        15
#define PSAB_PINK        16
#define PSAB_TRIANGLE    13
#define PSAB_CIRCLE      14
#define PSAB_CROSS       15
#define PSAB_SQUARE      16

//PORT and PIN presets
#define PB1 1
#define PB2 2
#define PB3 3
#define PB4 4
#define PB5 5

// pins for arduino
#define CONTROLLER_DAT_PIN PB5 // brown wire
#define CONTROLLER_CMD_PIN PB4 // orange wire
#define CONTROLLER_ATT_PIN PB3 // yellow wire
#define CONTROLLER_CLK_PIN PB2 // blue wire

// this pin is used only in guitar emulation mode
#define CONTROLLER_ACK_PIN PB1

#define INPUT_MODE(port,pin) (DDR ## port &= ~(1<<pin))
#define OUTPUT_MODE(port,pin) (DDR ## port |= (1<<pin))
#define CLEAR(port,pin) (PORT ## port &= ~(1<<pin))
#define SET(port,pin) (PORT ## port |= (1<<pin))
#define TOGGLE(port,pin) (PORT ## port ^= (1<<pin))
#define READ(port,pin) (PIN ## port & (1<<pin)) 



class PS2X 
{
	public:
		boolean Button(uint16_t);
		unsigned int ButtonDataByte();
		boolean NewButtonState();
		boolean NewButtonState(unsigned int);
		boolean ButtonPressed(unsigned int);
		boolean ButtonReleased(unsigned int);
		void read_gamepad();
		byte readType();
		byte config_gamepad();
		byte Analog(byte);
		void emulateGuitar();
	
	private:
		unsigned char _gamepad_shiftinout (char);
		unsigned char _gamepad_shiftinout (char,boolean);
		unsigned char PS2data[21];
		void sendCommandString(byte*, byte);
		void reconfig_gamepad();
		unsigned char i;
		unsigned int last_buttons;
		unsigned int buttons;
		unsigned long last_read;
		byte read_delay;
		byte controller_type;
		//byte const[5];
};

#endif



