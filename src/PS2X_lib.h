// $$$$$$$$$$$$ DEBUG ENABLE SECTION $$$$$$$$$$$$$$$$
// to debug ps2 controller, uncomment these two lines to print out debug to uart
//#define PS2X_DEBUG
//#define PS2X_COM_DEBUG

// https://store.curiousinventor.com/guides/PS2

#ifndef PS2X_lib_h
#define PS2X_lib_h

//~ #include <stdio.h>
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

/** \brief Type that is used to report button presses
 *
 * This can be used with the PSB_* values from PS2X_lib, and cast from/to
 * values of that type.
 */
typedef unsigned int PsxButtons;

	static const byte enter_config[] = {0x01, 0x43, 0x00, 0x01, 0x00};
	static const byte exit_config[] = {0x01, 0x43, 0x00, 0x00, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A};
	static const byte set_mode[] = {0x01, 0x44, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00};
	static const byte type_read[] = {0x01, 0x45, 0x00, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A};
	static const byte enable_rumble[] = {0x01, 0x4D, 0x00, 0x00, 0x01};
	static const byte set_bytes_large[] = {0x01, 0x4F, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x00, 0x00};
	


template <uint8_t PIN_CLK, uint8_t PIN_CMD, uint8_t PIN_ATT, uint8_t PIN_DAT>
class PS2X {
private:
	DigitalPin<PIN_CLK> clk;
	DigitalPin<PIN_CMD> cmd;
	DigitalPin<PIN_ATT> att;
	DigitalPin<PIN_DAT> dat;

	byte pollData[21];

	PsxButtons last_buttons;
	PsxButtons buttons;
	unsigned long last_read;
	byte readDelay;
	byte controllerType;
	boolean rumbleEnabled;
	boolean analogButtonsEnabled;

	byte shiftInOut (const byte out) const {
		byte in = 0;

		for (byte i = 0; i < 8; i++) {
			if (bitRead (out, i)) {
				cmd.high ();
			} else {
				cmd.low ();
			}

			clk.low ();
			delayMicroseconds (CTRL_CLK);

			if (dat) {
				bitSet (in, i);
			}

			clk.high ();
#if CTRL_CLK_HIGH
			delayMicroseconds (CTRL_CLK_HIGH);
#endif
		}

		cmd.high ();
		delayMicroseconds (CTRL_BYTE_DELAY);

		return in;
	}

	byte shiftInOut (const byte *out, byte *in, const byte len) const {
		for (byte i = 0; i < len; ++i) {
			byte tmp = shiftInOut (out[i]);
			if (in != NULL) {
				in[i] = tmp;
			}
		}
	}

	//~ __attribute__((always_inline))
	void sendCommandString (const byte *out, byte *in, const byte len) const {
		att.low (); // low enable joystick
		delayMicroseconds (CTRL_BYTE_DELAY);

		//~ for (byte i = 0; i < len; ++i) {
			//~ byte tmp = shiftInOut (out[i]);
			//~ if (in != NULL) {
				//~ in[i] = tmp;
			//~ }
		//~ }
		shiftInOut (out, in, len);

		att.high (); //high disable joystick
		delay (readDelay); //wait a few

#ifdef PS2X_COM_DEBUG
		Serial.println ("OUT:IN Configure");

		for (byte i = 0; i < len; ++i) {
			Serial.print (out[i], HEX);
			Serial.print (":");
			Serial.print (in[i], HEX);
			Serial.print (" ");
		}

		Serial.println ();
#endif
	}


public:
	PS2X (): last_buttons (0), buttons (0), last_read (0), readDelay (0),
		     controllerType (0), rumbleEnabled (false),
		     analogButtonsEnabled (false) {
	}
	
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
		read ();
		read ();

		//see if it talked - see if mode came back.
		//If still anything but 41, 73 or 79, then it's not talking
		if (pollData[1] != 0x41 && pollData[1] != 0x42 && pollData[1] != 0x73
				&& pollData[1] != 0x79) {
#ifdef PS2X_DEBUG
			Serial.println ("Controller mode not matched or no controller found");
			Serial.print ("Expected 0x41, 0x42, 0x73 or 0x79, but got ");
			Serial.println (pollData[1], HEX);
#endif
			return 1; //return error code 1
		}

		//try setting mode, increasing delays if need be
		readDelay = 1;

		for (byte y = 0; y <= 10; ++y) {
			sendCommandString (enter_config, NULL, sizeof (enter_config)); //start config run

			//read type
			//~ delayMicroseconds (CTRL_BYTE_DELAY);

			sendCommandString (type_read, temp, sizeof (type_read));
			controllerType = temp[3];

			// Enable analog mode
			sendCommandString (set_mode, NULL, sizeof (set_mode));

			// Leave config mode
			sendCommandString (exit_config, NULL, sizeof (exit_config));

			read ();

			// FIXME: Check this
			// If mode (2.data) is 41, the packet only contains 5 bytes, if mode == 0x73, 9 bytes are returned.
			if (pollData[1] == 0x73) {
				break;
			}

			if (y == 10) {
#ifdef PS2X_DEBUG
				Serial.println ("Controller not accepting commands");
				Serial.print ("mode still set at");
				Serial.println (pollData[1], HEX);
#endif
				return 2; //exit function with error
			}

			readDelay += 1; //add 1ms to readDelay
		}

		return 0; //no error if here
	}

	void enableRumble () {
		sendCommandString (enter_config, NULL, sizeof (enter_config));
		sendCommandString (enable_rumble, NULL, sizeof (enable_rumble));
		sendCommandString (exit_config, NULL, sizeof (exit_config));
		rumbleEnabled = true;
	}

	boolean enablePressures () {
		sendCommandString (enter_config, NULL, sizeof (enter_config));
		sendCommandString (set_bytes_large, NULL, sizeof (set_bytes_large));
		sendCommandString (exit_config, NULL, sizeof (exit_config));

		read ();
		read ();

		if (pollData[1] == 0x79) {
			analogButtonsEnabled = true;
		}

		// if 0x73 Controller refusing to enter Pressures mode?

		return analogButtonsEnabled;
	}

	boolean read (const boolean motor1 = false, const byte motor2 = 0) {
		const double temp = millis() - last_read;
		if (temp > 1500) {
			//waited too long
			reconfig_gamepad ();
		}

		if (temp < readDelay) {
			//waited too little FIXME: just return previous data?
			delay (readDelay - temp);
		}

		byte motor2fixed = 0;
		if (motor2 > 0x00) {
			motor2fixed = map (motor2, 0, 255, 0x40, 0xFF);    //nothing below 40 will make it spin
		}

		const byte dword[9] = {0x01, 0x42, 0, motor1, motor2fixed, 0, 0, 0, 0};

		// Try a few times to get valid data...
		for (byte retry = 0; retry < 5; ++retry) {
			//~ cmd.high ();
			//~ clk.high ();
			att.low ();	// low enable joystick

			delayMicroseconds (CTRL_BYTE_DELAY);

			//Send the command to send button and joystick data;
			shiftInOut (dword, pollData, sizeof (dword));

			if (pollData[1] == 0x79) {
				//if controller is in full data mode, get the rest of data
				for (byte i = 0; i < 12; ++i) {
					// Just send zeros
					pollData[i + sizeof (dword)] = shiftInOut (0);
				}
			}

			att.high (); // HI disable joystick

			// Check to see if we received valid data or not.
			// We should be in analog mode for our data to be valid (analog == 0x7_)
			if ((pollData[1] & 0xf0) == 0x70) {
				break;
			}

			// If we got to here, we are not in analog mode, try to recover...
			reconfig_gamepad (); // try to get back into Analog mode.
			delay (readDelay);
		}

		// If we get here and still not in analog mode (=0x7_), try increasing the readDelay...
		if ((pollData[1] & 0xf0) != 0x70) {
			if (readDelay < 10) {
				++readDelay;    // see if this helps out...
			}
		}

#ifdef PS2X_COM_DEBUG
		Serial.print ("OUT:IN ");

		for (byte i = 0; i < 9; i++) {
			Serial.print (dword[i], HEX);
			Serial.print (":");
			Serial.print (pollData[i], HEX);
			Serial.print (" ");
		}

		for (byte i = 0; i < 12; i++) {
			Serial.print (dword2[i], HEX);
			Serial.print (":");
			Serial.print (pollData[i + 9], HEX);
			Serial.print (" ");
		}

		Serial.println ("");
#endif

		last_buttons = buttons; //store the previous buttons states

		//store as one value for multiple functions
		//~ memcpy (&buttons, pollData + 3, sizeof (uint16_t));
		byte *tmp = reinterpret_cast<byte *> (&buttons);
		tmp[0] = pollData[3];
		tmp[1] = pollData[4];

		last_read = millis();

		return ((pollData[1] & 0xf0) == 0x70);  // 1 = OK = analog mode - 0 = NOK
	}
	
		// FIXME: Maybe this can be merged with begin?
	void reconfig_gamepad() {
		sendCommandString (enter_config, NULL, sizeof (enter_config));
		sendCommandString (set_mode, NULL, sizeof (set_mode));

		if (rumbleEnabled) {
			sendCommandString (enable_rumble, NULL, sizeof (enable_rumble));
		}

		if (analogButtonsEnabled) {
			sendCommandString (set_bytes_large, NULL, sizeof (set_bytes_large));
		}

		sendCommandString (exit_config, NULL, sizeof (exit_config));
	}

	byte readType() {
		/*
		  byte temp[sizeof(type_read)];

		  sendCommandString(enter_config, NULL, sizeof(enter_config));

		  delayMicroseconds(CTRL_BYTE_DELAY);

		  CMD_SET();
		  CLK_SET();
		  ATT_CLR(); // low enable joystick

		  delayMicroseconds(CTRL_BYTE_DELAY);

		  for (int i = 0; i<9; i++) {
			temp[i] = shiftInOut(type_read[i]);
		  }

		  sendCommandString(exit_config, NULL, sizeof(exit_config));

		  if(temp[3] == 0x03)
			return 1;
		  else if(temp[3] == 0x01)
			return 2;

		  return 0;
		*/

#ifdef PS2X_COM_DEBUG
		Serial.print ("Controller Type: ");
		Serial.println (controllerType, HEX);
#endif

		if (controllerType == 0x03) {
			return 1;
		} else if (controllerType == 0x01 && pollData[1] == 0x42) {
			return 4;
		}  else if (controllerType == 0x01/* && pollData[1] != 0x42*/) {
			return 2;
		} else if (controllerType == 0x0C) {
			return 3;    //2.4G Wireless Dual Shock PS2 Game Controller
		}

		return 0;
	}

	PsxButtons buttonWord () const {
		return ~buttons;
	}

	boolean button (const PsxButtons bw, const PsxButtons b) const {
		return ((bw & b) != 0);
	}

	//will be TRUE if button is being pressed
	boolean button (const PsxButtons b) const {
		return button (~buttons, b);
	}

	// Will return true if ANY button was JUST pressed OR released
	boolean newButtonState () const {
		return ((last_buttons ^ buttons) > 0);
	}

	//will be TRUE if button was JUST pressed OR released
	boolean newButtonState (const PsxButtons button) const {
		return (((last_buttons ^ buttons) & button) > 0);
	}

	//will be TRUE if button was JUST pressed
	boolean buttonPressed (const PsxButtons button) const {
		return (newButtonState (button) & Button (button));
	}

	//will be TRUE if button was JUST released
	boolean buttonReleased (const PsxButtons button) const {
		return ((newButtonState (button)) & ((~last_buttons & button) > 0));
	}

	byte analog (const byte button) const {
		return pollData[button];
	}

#if 1
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
		return buttonWord ();
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
#endif
};


#endif
