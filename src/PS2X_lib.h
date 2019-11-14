/******************************************************************
*  Super amazing PS2 controller Arduino Library v1.8
*       details and example sketch:
*           http://www.billporter.info/?p=240
*
*    Original code by Shutter on Arduino Forums
*
*    Revamped, made into lib by and supporting continued development:
*              Bill Porter
*              www.billporter.info
*
*    Contributers:
*       Eric Wetzel (thewetzel@gmail.com)
*       Kurt Eckhardt
*
*  Lib version history
*    0.1 made into library, added analog stick support.
*    0.2 fixed config_gamepad miss-spelling
*        added new functions:
*          NewButtonState();
*          NewButtonState(unsigned int);
*          ButtonPressed(unsigned int);
*          ButtonReleased(unsigned int);
*        removed 'PS' from beginning of ever function
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
*   1.3
*       Changed clock back to 50kHz. CuriousInventor says it's suppose to be 500kHz, but doesn't seem to work for everybody.
*   1.4
*       Removed redundant functions.
*       Fixed mode check to include two other possible modes the controller could be in.
*       Added debug code enabled by compiler directives. See below to enable debug mode.
*       Added button definitions for shapes as well as colors.
*   1.41
*       Some simple bug fixes
*       Added Keywords.txt file
*   1.5
*       Added proper Guitar Hero compatibility
*       Fixed issue with DEBUG mode, had to send serial at once instead of in bits
*   1.6
*       Changed config_gamepad() call to include rumble and pressures options
*           This was to fix controllers that will only go into config mode once
*           Old methods should still work for backwards compatibility
*    1.7
*       Integrated Kurt's fixes for the interrupts messing with servo signals
*       Reorganized directory so examples show up in Arduino IDE menu
*    1.8
*       Added Arduino 1.0 compatibility.
*    1.9
*       Kurt - Added detection and recovery from dropping from analog mode, plus
*       integrated Chipkit (pic32mx...) support
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

// $$$$$$$$$$$$ DEBUG ENABLE SECTION $$$$$$$$$$$$$$$$
// to debug ps2 controller, uncomment these two lines to print out debug to uart
//#define PS2X_DEBUG
//#define PS2X_COM_DEBUG

// https://store.curiousinventor.com/guides/PS2

#ifndef PS2X_lib_h
#define PS2X_lib_h

#include <stdio.h>
#include <stdint.h>
#include <Arduino.h>
#include <DigitalIO.h>

#define CTRL_CLK        20
#define CTRL_BYTE_DELAY 4


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
#define UP_STRUM        0x0010
#define DOWN_STRUM      0x0040
#define LEFT_STRUM      0x0080
#define RIGHT_STRUM     0x0020
#define STAR_POWER      0x0100
#define GREEN_FRET      0x0200
#define YELLOW_FRET     0x1000
#define RED_FRET        0x2000
#define BLUE_FRET       0x4000
#define ORANGE_FRET     0x8000
#define WHAMMY_BAR      8

//These are stick values
#define PSS_RX 5
#define PSS_RY 6
#define PSS_LX 7
#define PSS_LY 8

//These are analog buttons
#define PSAB_PAD_RIGHT    9
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

	static const byte enter_config[] = {0x01, 0x43, 0x00, 0x01, 0x00};
	static const byte set_mode[] = {0x01, 0x44, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00};
	static const byte set_bytes_large[] = {0x01, 0x4F, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x00, 0x00};
	static const byte exit_config[] = {0x01, 0x43, 0x00, 0x00, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A};
	static const byte enable_rumble[] = {0x01, 0x4D, 0x00, 0x00, 0x01};
	static const byte type_read[] = {0x01, 0x45, 0x00, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A};


template <uint8_t PIN_CLK, uint8_t PIN_CMD, uint8_t PIN_ATT, uint8_t PIN_DAT>
class PS2X {
private:
	DigitalPin<PIN_CLK> clk;
	DigitalPin<PIN_CMD> cmd;
	DigitalPin<PIN_ATT> att;
	DigitalPin<PIN_DAT> dat;

	unsigned char PS2data[21];

	unsigned char i;
	unsigned int last_buttons;
	unsigned int buttons;
	unsigned long last_read;
	byte read_delay;
	byte controller_type;
	boolean en_Rumble;
	boolean en_Pressures;

	byte _gamepad_shiftinout (const byte b) {
		byte tmp = 0;

		for (byte i = 0; i < 8; i++) {
			if (bitRead (b, i)) {
				cmd.high ();
			} else {
				cmd.low ();
			}

			clk.low ();
			delayMicroseconds (CTRL_CLK);

			if (dat.read ()) {
				bitSet (tmp, i);
			}

			clk.high ();
#if CTRL_CLK_HIGH
			delayMicroseconds (CTRL_CLK_HIGH);
#endif
		}

		cmd.high ();
		delayMicroseconds (CTRL_BYTE_DELAY);

		return tmp;
	}

	void sendCommandString (byte string[], byte len) {
#ifdef PS2X_COM_DEBUG
		byte temp[len];
		att.low (); // low enable joystick
		delayMicroseconds (CTRL_BYTE_DELAY);

		for (int y = 0; y < len; y++) {
			temp[y] = _gamepad_shiftinout (string[y]);
		}

		att.high (); //high disable joystick
		delay (read_delay); //wait a few

		Serial.println ("OUT:IN Configure");

		for (int i = 0; i < len; i++) {
			Serial.print (string[i], HEX);
			Serial.print (":");
			Serial.print (temp[i], HEX);
			Serial.print (" ");
		}

		Serial.println ("");
#else
		att.low (); // low enable joystick
		delayMicroseconds (CTRL_BYTE_DELAY);

		for (int y = 0; y < len; y++) {
			_gamepad_shiftinout (string[y]);
		}

		att.high (); //high disable joystick
		delay (read_delay);                 //wait a few
#endif
	}


public:
	byte begin () {
		byte temp[sizeof (type_read)];

		//configure ports
		clk.mode (OUTPUT);
		att.mode (OUTPUT);
		cmd.mode (OUTPUT);
		dat.config (INPUT, HIGH);		//enable pull-up

		cmd.high ();
		clk.high ();

		//new error checking. First, read gamepad a few times to see if it's talking
		read_gamepad ();
		read_gamepad ();

		//see if it talked - see if mode came back.
		//If still anything but 41, 73 or 79, then it's not talking
		if (PS2data[1] != 0x41 && PS2data[1] != 0x42 && PS2data[1] != 0x73
				&& PS2data[1] != 0x79) {
#ifdef PS2X_DEBUG
			Serial.println ("Controller mode not matched or no controller found");
			Serial.print ("Expected 0x41, 0x42, 0x73 or 0x79, but got ");
			Serial.println (PS2data[1], HEX);
#endif
			return 1; //return error code 1
		}

		//try setting mode, increasing delays if need be
		read_delay = 1;

		for (byte y = 0; y <= 10; ++y) {
			sendCommandString (enter_config, sizeof (enter_config)); //start config run

			//read type
			delayMicroseconds (CTRL_BYTE_DELAY);

			cmd.high ();
			clk.high ();
			att.low (); // low enable joystick

			delayMicroseconds (CTRL_BYTE_DELAY);

			for (byte i = 0; i < sizeof (type_read); i++) {
				temp[i] = _gamepad_shiftinout (type_read[i]);
			}

			att.high (); // HI disable joystick

			controller_type = temp[3];

			// Enable analog mode
			sendCommandString (set_mode, sizeof (set_mode));

			// Leave config mode
			sendCommandString (exit_config, sizeof (exit_config));

			read_gamepad();

			// FIXME: Check this
			// If mode (2.data) is 41, the packet only contains 5 bytes, if mode == 0x73, 9 bytes are returned.
			if (PS2data[1] == 0x73) {
				break;
			}

			if (y == 10) {
#ifdef PS2X_DEBUG
				Serial.println ("Controller not accepting commands");
				Serial.print ("mode still set at");
				Serial.println (PS2data[1], HEX);
#endif
				return 2; //exit function with error
			}

			read_delay += 1; //add 1ms to read_delay
		}

		return 0; //no error if here
	}

	void enableRumble () {
		sendCommandString (enter_config, sizeof (enter_config));
		sendCommandString (enable_rumble, sizeof (enable_rumble));
		sendCommandString (exit_config, sizeof (exit_config));
		en_Rumble = true;
	}

	boolean enablePressures () {
		sendCommandString (enter_config, sizeof (enter_config));
		sendCommandString (set_bytes_large, sizeof (set_bytes_large));
		sendCommandString (exit_config, sizeof (exit_config));

		read_gamepad ();
		read_gamepad ();

		if (PS2data[1] == 0x79) {
			en_Pressures = true;
		}

		// if 0x73 Controller refusing to enter Pressures mode?

		return en_Pressures;
	}

	boolean read (boolean motor1, byte motor2) {
		double temp = millis() - last_read;
		if (temp > 1500) {
			//waited too long
			reconfig_gamepad ();
		}

		if (temp < read_delay) {
			//waited too little FIXME: just return previous data?
			delay (read_delay - temp);
		}

		if (motor2 != 0x00) {
			motor2 = map (motor2, 0, 255, 0x40, 0xFF);    //noting below 40 will make it spin
		}

		const byte dword[9] = {0x01, 0x42, 0, motor1, motor2, 0, 0, 0, 0};

		// Try a few times to get valid data...
		for (byte retry = 0; retry < 5; ++retry) {
			cmd.high ();
			clk.high ();
			att.low ();	// low enable joystick

			delayMicroseconds (CTRL_BYTE_DELAY);

			//Send the command to send button and joystick data;
			for (byte i = 0; i < sizeof (dword); ++i) {
				PS2data[i] = _gamepad_shiftinout (dword[i]);
			}

			if (PS2data[1] == 0x79) {
				//if controller is in full data return mode, get the rest of data
				for (byte i = 0; i < 12; ++i) {
					// Just send zeros
					PS2data[i + sizeof (dword)] = _gamepad_shiftinout (0);
				}
			}

			att.high (); // HI disable joystick

			// Check to see if we received valid data or not.
			// We should be in analog mode for our data to be valid (analog == 0x7_)
			if ((PS2data[1] & 0xf0) == 0x70) {
				break;
			}

			// If we got to here, we are not in analog mode, try to recover...
			reconfig_gamepad (); // try to get back into Analog mode.
			delay (read_delay);
		}

		// If we get here and still not in analog mode (=0x7_), try increasing the read_delay...
		if ((PS2data[1] & 0xf0) != 0x70) {
			if (read_delay < 10) {
				++read_delay;    // see if this helps out...
			}
		}

#ifdef PS2X_COM_DEBUG
		Serial.print ("OUT:IN ");

		for (byte i = 0; i < 9; i++) {
			Serial.print (dword[i], HEX);
			Serial.print (":");
			Serial.print (PS2data[i], HEX);
			Serial.print (" ");
		}

		for (byte i = 0; i < 12; i++) {
			Serial.print (dword2[i], HEX);
			Serial.print (":");
			Serial.print (PS2data[i + 9], HEX);
			Serial.print (" ");
		}

		Serial.println ("");
#endif

		last_buttons = buttons; //store the previous buttons states

		//store as one value for multiple functions
		memcpy (&buttons, PS2data + 3, sizeof (uint16_t));

		last_read = millis();

		return ((PS2data[1] & 0xf0) == 0x70);  // 1 = OK = analog mode - 0 = NOK
	}


	unsigned int buttonDataByte() {
		return ~buttons;
	}

	boolean button (uint16_t buttonWord, uint16_t b) {
		return ((buttonWord & b) != 0);
	}

	//will be TRUE if button is being pressed
	boolean button (uint16_t b) {
		return button (~buttons, b);
	}

	// Will return true if ANY button was JUST pressed OR released
	boolean newButtonState() {
		return ((last_buttons ^ buttons) > 0);
	}

	//will be TRUE if button was JUST pressed OR released
	boolean newButtonState (unsigned int button) {
		return (((last_buttons ^ buttons) & button) > 0);
	}

	//will be TRUE if button was JUST pressed
	boolean buttonPressed (unsigned int button) {
		return (newButtonState (button) & Button (button));
	}

	//will be TRUE if button was JUST released
	boolean buttonReleased (unsigned int button) {
		return ((bewButtonState (button)) & ((~last_buttons & button) > 0));
	}

	byte analog (byte button) {
		return PS2data[button];
	}

	// FIXME: Maybe this can be merged with begin?
	void reconfig_gamepad() {
		sendCommandString (enter_config, sizeof (enter_config));
		sendCommandString (set_mode, sizeof (set_mode));

		if (en_Rumble) {
			sendCommandString (enable_rumble, sizeof (enable_rumble));
		}

		if (en_Pressures) {
			sendCommandString (set_bytes_large, sizeof (set_bytes_large));
		}

		sendCommandString (exit_config, sizeof (exit_config));
	}

	byte readType() {
		/*
		  byte temp[sizeof(type_read)];

		  sendCommandString(enter_config, sizeof(enter_config));

		  delayMicroseconds(CTRL_BYTE_DELAY);

		  CMD_SET();
		  CLK_SET();
		  ATT_CLR(); // low enable joystick

		  delayMicroseconds(CTRL_BYTE_DELAY);

		  for (int i = 0; i<9; i++) {
			temp[i] = _gamepad_shiftinout(type_read[i]);
		  }

		  sendCommandString(exit_config, sizeof(exit_config));

		  if(temp[3] == 0x03)
			return 1;
		  else if(temp[3] == 0x01)
			return 2;

		  return 0;
		*/

#ifdef PS2X_COM_DEBUG
		Serial.print ("Controller_type: ");
		Serial.println (controller_type, HEX);
#endif

		if (controller_type == 0x03) {
			return 1;
		} else if (controller_type == 0x01 && PS2data[1] == 0x42) {
			return 4;
		}  else if (controller_type == 0x01 && PS2data[1] != 0x42) {
			return 2;
		} else if (controller_type == 0x0C) {
			return 3;    //2.4G Wireless Dual Shock PS2 Game Controller
		}

		return 0;
	}

	// Backwards compatibility
	byte config_gamepad (boolean pressures = false, boolean rumble = false) {
		byte ret = begin ();

		if (pressures) {
			if (!enablePressures ()) {
				ret = 3;
			}
		}

		if (rumble) {
			enableRumble ();
		}

		return ret;
	}

	boolean read_gamepad (boolean motor1 = false, byte motor2 = 0) {
		return read (motor1, motor2);
	}

	unsigned int ButtonDataByte() {
		return buttonDataByte ();
	}

	boolean Button (uint16_t buttonWord, uint16_t b) {
		return button (buttonWord, b);
	}

	//will be TRUE if button is being pressed
	boolean Button (uint16_t b) {
		return button (b);
	}

	boolean NewButtonState() {
		return newButtonState ();
	}

	//will be TRUE if button was JUST pressed OR released
	boolean NewButtonState (unsigned int button) {
		return newButtonState (button);
	}

	//will be TRUE if button was JUST pressed
	boolean ButtonPressed (unsigned int button) {
		return buttonPressed (button);
	}

	//will be TRUE if button was JUST released
	boolean ButtonReleased (unsigned int button) {
		return buttonReleased (button);
	}

	byte Analog (byte button) {
		return analog (button);
	}
};


#endif
