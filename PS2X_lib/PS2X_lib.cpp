#include "PS2X_lib.h"
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <avr/io.h>
#if ARDUINO > 22
#include "Arduino.h"
#else
#include "WProgram.h"
#include "pins_arduino.h"
#endif




//static byte read_data[]={0x01,0x42,0x00,0x00,0x00};
static byte enter_config[]={0x01,0x43,0x00,0x01,0x00};
static byte set_analogue[]={0x01,0x44,0x00,0x01,0x03,0x00,0x00,0x00,0x00}; 
//static byte set_digital[]={0x01,0x44,0x00,0x00,0x03,0x00,0x00,0x00,0x00}; 
//static byte set_bytes_large[]={0x01,0x4F,0x00,0xFF,0xFF,0x03,0x00,0x00,0x00};
static byte exit_config[]={0x01,0x43,0x00,0x00,0x5A,0x5A,0x5A,0x5A,0x5A};
//static byte enable_rumble[]={0x01,0x4D,0x00,0x00,0x01};
static byte type_read[]={0x01,0x45,0x00,0x5A,0x5A,0x5A,0x5A,0x5A,0x5A};
//
static byte const1_read[]={0x01,0x46,0x00,0x00,0x5A,0x5A,0x5A,0x5A,0x5A};
static byte const2_read[]={0x01,0x46,0x00,0x01,0x5A,0x5A,0x5A,0x5A,0x5A};
static byte const3_read[]={0x01,0x47,0x00,0x00,0x5A,0x5A,0x5A,0x5A,0x5A};
static byte const4_read[]={0x01,0x4C,0x00,0x00,0x5A,0x5A,0x5A,0x5A,0x5A};
static byte const5_read[]={0x01,0x4C,0x00,0x01,0x5A,0x5A,0x5A,0x5A,0x5A};
//

boolean PS2X::NewButtonState() 
{
	return ((last_buttons ^ buttons) > 0);
}

boolean PS2X::NewButtonState(unsigned int button) 
{
	return (((last_buttons ^ buttons) & button) > 0);
}

boolean PS2X::ButtonPressed(unsigned int button) 
{
	return(NewButtonState(button) & Button(button));
}

boolean PS2X::ButtonReleased(unsigned int button) 
{
	return((NewButtonState(button)) & ((~last_buttons & button) > 0));
}
  
boolean PS2X::Button(uint16_t button) 
{
	return ((~buttons & button) > 0);
}

unsigned int PS2X::ButtonDataByte() 
{
	return (~buttons);
}

byte PS2X::Analog(byte button) 
{
	return PS2data[button];
}

unsigned char PS2X::_gamepad_shiftinout(char byte) 
{
	return _gamepad_shiftinout(byte, false);
}

/*
 * Tom Price says:
 *
 * o  Some extra bits added to the serial handshaking protocol
 *    Most saliently control of the ACK line
 */
unsigned char PS2X::_gamepad_shiftinout(char byte, boolean acknowledge) 
{
	uint8_t old_sreg = SREG;        // *** KJE *** save away the current state of interrupts
	unsigned char tmp = 0;
	cli();                          // *** KJE *** disable for now
	SET(B,CONTROLLER_ACK_PIN);	// Tom Price: ACK off
	for (i=0;i<8;i++) 
	{
		if ( byte & _BV(i) ) 
		{
			SET(B,CONTROLLER_CMD_PIN);
		}
		else  
		{
			CLEAR(B,CONTROLLER_CMD_PIN);
		}
		CLEAR(B,CONTROLLER_CLK_PIN);

		SREG = old_sreg;  // *** *** KJE *** *** Interrupts may be enabled again 
		delayMicroseconds(CTRL_CLK);
		cli();	// *** KJE ***

		if ( READ(B,CONTROLLER_DAT_PIN) ) 
		{
			tmp |= _BV(i);
		}
		SET(B,CONTROLLER_CLK_PIN);
	}
	SET(B,CONTROLLER_CMD_PIN);
	if (acknowledge)
	{
		CLEAR(B,CONTROLLER_ACK_PIN);	// Tom Price: ACK on
	}
	SREG = old_sreg;  // *** *** KJE *** *** Interrupts may be enabled again 
	delayMicroseconds(CTRL_BYTE_DELAY);
	SET(B,CONTROLLER_ACK_PIN);	// Tom Price: ACK offo
	return tmp;
}

void PS2X::read_gamepad() 
{	
	double temp = millis() - last_read;
	uint8_t old_sreg = SREG;        // *** KJE **** save away the current state of interrupts - *** *** KJE *** ***

	if (temp > 1500) //waited too long
	{
		reconfig_gamepad();
	}  
	if (temp < read_delay)  //waited too short
	{
		delay(read_delay - temp);
	}
	last_buttons = buttons; //store the previous buttons states

	cli();	//*** KJE ***  
	SET(B,CONTROLLER_CMD_PIN);
	SET(B,CONTROLLER_CLK_PIN);
	CLEAR(B,CONTROLLER_ATT_PIN); // low enable joystick
	SREG = old_sreg;  // *** KJE *** - Interrupts may be enabled again

	delayMicroseconds(CTRL_BYTE_DELAY);
	//Send the command to send button and joystick data;
	char dword[9] = {0x01,0x42,0,0,0,0,0,0,0};
	byte dword2[12] = {0,0,0,0,0,0,0,0,0,0,0,0};

	for (int i = 0; i<9; i++) 
	{
		PS2data[i] = _gamepad_shiftinout(dword[i]);
	}
	if (PS2data[1] == 0x79) //if controller is in full data return mode, get the rest of data
	{  
		for (int i = 0; i<12; i++) 	
		{
			PS2data[i+9] = _gamepad_shiftinout(dword2[i]);
		}
	}

	cli();
	SET(B,CONTROLLER_ATT_PIN); // HI disable joystick
	SREG = old_sreg;  // Interrupts may be enabled again    

	#ifdef PS2X_COM_DEBUG
	Serial.println("OUT:IN");	
	{
		for (int i=0; i<9; i++)
		{
			Serial.print(dword[i], HEX);
			Serial.print(":");
			Serial.print(PS2data[i], HEX);
			Serial.print(" ");
		}
	}
	if (PS2data[1] == 0x79)
	{
		for (int i = 0; i<12; i++) 
		{
			Serial.print(dword2[i], HEX);
			Serial.print(":");
			Serial.print(PS2data[i+9], HEX);
			Serial.print(" ");
		}
	}
	Serial.println("");	
	#endif

	buttons = *(uint16_t*)(PS2data+3);   //store as one value for multiple functions
	last_read = millis();
}

byte PS2X::config_gamepad()
{
	uint8_t old_sreg = SREG;        // *** KJE *** save away the current state of interrupts
	byte temp[sizeof(type_read)];

	//configure pins
	OUTPUT_MODE(B,CONTROLLER_CLK_PIN); 
	OUTPUT_MODE(B,CONTROLLER_CMD_PIN); 
	OUTPUT_MODE(B,CONTROLLER_ATT_PIN); 
	INPUT_MODE(B,CONTROLLER_DAT_PIN); 

	//enable pull-up resistor
	SET(B,CONTROLLER_DAT_PIN); 

	cli();                          // *** KJE *** disable for now
	SET(B,CONTROLLER_CMD_PIN); 
	SET(B,CONTROLLER_CLK_PIN);
	SREG = old_sreg;  // *** *** KJE *** *** Interrupts may be enabled again 

	//new error checking. First, read gamepad a few times to see if it's talking
	read_gamepad();
	read_gamepad();

	//see if it talked
	if (PS2data[1] != 0x41 && PS2data[1] != 0x73 && PS2data[1] != 0x79)
	{ 
		//see if mode came back. If still anything but 41, 73 or 79, then it's not talking
		#ifdef PS2X_DEBUG
		Serial.println("Controller mode not matched or no controller found");
		Serial.print("Expected 0x41 or 0x73, got ");
		Serial.println(PS2data[1], HEX);
		#endif
		return 1; //return error code 1
	}

	//try setting mode, increasing delays if need be. 
	read_delay = 1;

	for (int y = 0; y <= 10; y++)
	{
		//sendCommandString(poll_once, sizeof(poll_once)); // poll once
		sendCommandString(enter_config, sizeof(enter_config)); // start config run

		// read type
		delayMicroseconds(CTRL_BYTE_DELAY);
		cli();                          // *** KJE *** disable for now
		SET(B,CONTROLLER_CMD_PIN);
		SET(B,CONTROLLER_CLK_PIN);
		CLEAR(B,CONTROLLER_ATT_PIN); // low enable joystick
		SREG = old_sreg;  // *** *** KJE *** *** Interrupts may be enabled again 
		delayMicroseconds(CTRL_BYTE_DELAY);
		for (int i = 0; i<9; i++) 
		{
			temp[i] = _gamepad_shiftinout(type_read[i]);
		}
		#ifdef PS2X_COM_DEBUG
		Serial.println("OUT:IN Configure");
		for (int i=0; i<9; i++)
		{
			Serial.print(type_read[i], HEX);
			Serial.print(":");
			Serial.print(temp[i], HEX);
			Serial.print(" ");
		}
		Serial.println("");
		#endif
		cli();                          // *** KJE *** disable for now
		SET(B,CONTROLLER_ATT_PIN); // HI disable joystick
		SREG = old_sreg;  // *** *** KJE *** *** Interrupts may be enabled again 
		controller_type = temp[3];		
		
		sendCommandString(const1_read, sizeof(const1_read)); 
		sendCommandString(const2_read, sizeof(const2_read)); 
		sendCommandString(const3_read, sizeof(const3_read)); 
		sendCommandString(const4_read, sizeof(const4_read)); 
		sendCommandString(const5_read, sizeof(const5_read)); 
		sendCommandString(set_analogue, sizeof(set_analogue));
		sendCommandString(exit_config, sizeof(exit_config));

		read_gamepad();

		if 
		(
			(PS2data[1] == 0x73)		
		)
		{
			break;
		}

		if (y == 10)
		{
			#ifdef PS2X_DEBUG
			Serial.println("Controller not accepting commands");
			Serial.print("mode stil set at ");
			Serial.println(PS2data[1], HEX);
			#endif
			return 2; //exit function with error
		}

		read_delay += 1; //add 1ms to read_delay
	}
	return 0; //no error if here
}

void PS2X::sendCommandString(byte string[], byte len) 
{
	uint8_t old_sreg = SREG;        // *** KJE *** save away the current state of interrupts

	#ifdef PS2X_COM_DEBUG
	byte temp[len];
	cli();                          // *** KJE *** disable for now
	CLEAR(B,CONTROLLER_ATT_PIN); // low enable joystick
	SREG = old_sreg;  // *** *** KJE *** *** Interrupts may be enabled again 

	for (int y=0; y < len; y++)
	{
		temp[y] = _gamepad_shiftinout(string[y]);
	}

	cli();                          // *** KJE *** disable for now
	SET(B,CONTROLLER_ATT_PIN); //high disable joystick  
	SREG = old_sreg;  // *** *** KJE *** *** Interrupts may be enabled again 
	delay(read_delay);                  //wait a few

	Serial.println("OUT:IN Configure");
	for (int i=0; i<len; i++)
	{
		Serial.print(string[i], HEX);
		Serial.print(":");
		Serial.print(temp[i], HEX);
		Serial.print(" ");
	}
	Serial.println("");

	#else
	cli();                          // *** KJE *** disable for now
	CLEAR(B,CONTROLLER_ATT_PIN); // low enable joystick
	SREG = old_sreg;  // *** *** KJE *** *** Interrupts may be enabled again 
	for (int y=0; y < len; y++)
	{
		_gamepad_shiftinout(string[y]);
	}

	cli();                          // *** KJE *** disable for now
	SET(B,CONTROLLER_ATT_PIN); //high disable joystick  
	SREG = old_sreg;  // *** *** KJE *** *** Interrupts may be enabled again 
	delay(read_delay);                  //wait a few
	#endif
}

byte PS2X::readType() 
{
	if (controller_type == 0x03)
	{
		return 1;
	}
	else if (controller_type == 0x01)
	{
		return 2;
	}
	return 0;
}

void PS2X::reconfig_gamepad()
{
	sendCommandString(enter_config, sizeof(enter_config));
	sendCommandString(set_analogue, sizeof(set_analogue));
	sendCommandString(exit_config, sizeof(exit_config));
}

/*
 * Tom Price says:
 *
 * o  A new method PS2X::emulateGuitar() allowing the Arduino
 *    to emulate a Guitar Hero controller by handling the
 *    controller side of the serial communication protocol
 */
void PS2X::emulateGuitar()
{
	uint8_t mode = MODE_DIGITAL;
	uint8_t analogue = 0;
	uint8_t i, cmd[8];
	while( 1 )
	{
		// listen for start of command
		if ( _gamepad_shiftinout( 0xFF, false ) == 0x01 )
		{
			// get command
			cmd[0] = _gamepad_shiftinout( mode, true );
			cmd[1] = _gamepad_shiftinout( 0x5A, true );
			if ( cmd[0] == 0x42 )
			{
				// read controller data
				// return all buttons activated (except STAR POWER)
				cmd[2] = _gamepad_shiftinout( 0xF2, true );
				cmd[3] = _gamepad_shiftinout( 0x50, true );
				cmd[4] = _gamepad_shiftinout( 0x00, false );
				if ( mode == MODE_DIGITAL )
				{
					continue;
				}
				for ( i = 5; i <= 7; i++ )
				{
					cmd[i] = _gamepad_shiftinout( 0x00, i == 7 );
				}
				continue;
			} // cmd 0x42
			else if ( cmd[0] == 0x43 ) 
			{
				// read controller data and toggle configuration mode
				// return all buttons activated (except STAR POWER)
				cmd[2] = _gamepad_shiftinout( 0xF2, true );
				cmd[3] = _gamepad_shiftinout( 0x50, true );
				cmd[4] = _gamepad_shiftinout( 0x00, mode != MODE_DIGITAL );
				if ( mode == MODE_DIGITAL )
				{
					continue;
				}
				for ( i = 5; i <= 7; i++ )
				{
					cmd[i] = _gamepad_shiftinout( 0x00, i == 7 );
				}
				if ( cmd[2] == 1 )
				{
					mode = MODE_CONFIG;
				}
				if ( cmd[2] == 0 )
				{
					if ( analogue )
					{
						mode = MODE_ANALOGUE;
					}
					else
					{
						mode = MODE_DIGITAL;
					}
				}
				continue;
			} // cmd 0x43
			else if ( mode == MODE_CONFIG ) 
			{
				// configuration commands
				if ( cmd[0] == 0x44 )
				{
					for ( i = 2; i <= 7; i++ )
					{
						cmd[i] = _gamepad_shiftinout( 0x00, i == 7 );
					}
					if ( cmd[3] == 1 )
					{
						analogue = 1;
					}
					else if ( cmd[3] == 0 )
					{
						analogue = 0;
					}
					continue;
				} // cmd 44
				else if ( cmd[0] == 0x45 )
				{
					cmd[2] = _gamepad_shiftinout( 0x01, true ); // guitar hero
					cmd[3] = _gamepad_shiftinout( 0x02, true );
					cmd[4] = _gamepad_shiftinout( 0x01, true ); // LED on
					cmd[5] = _gamepad_shiftinout( 0x02, true );
					cmd[6] = _gamepad_shiftinout( 0x01, true );  
					cmd[7] = _gamepad_shiftinout( 0x00, false );  
					continue;
				} // cmd 45
				else if ( cmd[0] == 0x46 )
				{
					cmd[2] = _gamepad_shiftinout( 0x00, true );
					if ( cmd[2] == 0 )
					{
						cmd[3] = _gamepad_shiftinout( 0x00, true ); 
						cmd[4] = _gamepad_shiftinout( 0x01, true );
						cmd[5] = _gamepad_shiftinout( 0x02, true ); 
						cmd[6] = _gamepad_shiftinout( 0x00, true );
						cmd[7] = _gamepad_shiftinout( 0x0A, false );  
					}
					else if ( cmd[2] == 1 )
					{
						cmd[3] = _gamepad_shiftinout( 0x00, true ); 
						cmd[4] = _gamepad_shiftinout( 0x01, true );
						cmd[5] = _gamepad_shiftinout( 0x01, true ); 
						cmd[6] = _gamepad_shiftinout( 0x01, true );
						cmd[7] = _gamepad_shiftinout( 0x14, false ); 
					}
					else
					{
						for ( i = 2; i <= 7; i++ )
						{
							cmd[i] = _gamepad_shiftinout( 0x00, i == 7 );
						}
					}
					continue;
				} // cmd 46
				else if ( cmd[0] == 0x47 )
				{
					cmd[2] = _gamepad_shiftinout( 0x00, true );
					cmd[3] = _gamepad_shiftinout( 0x00, true ); 
					cmd[4] = _gamepad_shiftinout( 0x02, true );
					cmd[5] = _gamepad_shiftinout( 0x00, true ); 
					cmd[6] = _gamepad_shiftinout( 0x01, true );
					cmd[7] = _gamepad_shiftinout( 0x00, false );  
					continue;
				} // cmd 47
				else if ( cmd[0] == 0x4C )
				{
					cmd[2] = _gamepad_shiftinout( 0x00, true );
					if ( cmd[2] == 0 )
					{
						cmd[3] = _gamepad_shiftinout( 0x00, true ); 
						cmd[4] = _gamepad_shiftinout( 0x00, true );
						cmd[5] = _gamepad_shiftinout( 0x04, true ); 
						cmd[6] = _gamepad_shiftinout( 0x00, true );
						cmd[7] = _gamepad_shiftinout( 0x00, false );  
					}
					else if ( cmd[2] == 1 )
					{
						cmd[3] = _gamepad_shiftinout( 0x00, true ); 
						cmd[4] = _gamepad_shiftinout( 0x00, true );
						cmd[5] = _gamepad_shiftinout( 0x07, true ); 
						cmd[6] = _gamepad_shiftinout( 0x00, true );
						cmd[7] = _gamepad_shiftinout( 0x00, false); 
					}
					else
					{
						for ( i = 2; i <= 7; i++ )
						{
							cmd[i] = _gamepad_shiftinout( 0x00, i == 7 );
						}
					}
					continue;
				} // cmd 4C
				else if ( cmd[0] == 0x4D )
				{
					cmd[2] = _gamepad_shiftinout( 0x00, true );
					cmd[3] = _gamepad_shiftinout( 0x01, true ); 
					for ( i = 4; i <= 7; i++ )
					{
						cmd[i] = _gamepad_shiftinout( 0xFF, i == 7 );
					}  
					// supposed to do other configuration stuff
					continue;
				} // cmd 4D
				else if ( cmd[0] == 0x4F )
				{
					for ( i = 2; i <= 6; i++ )
					{
						cmd[i] = _gamepad_shiftinout( 0x00, true );
					}  
					cmd[7] = _gamepad_shiftinout( 0x5A, false ); 
					// supposed to do other configuration stuff
					continue;
				} // cmd 4F
			} // configuration commands
		}
	} // repeat forever
}
