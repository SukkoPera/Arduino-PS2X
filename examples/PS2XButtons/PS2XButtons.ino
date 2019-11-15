#include <avr/pgmspace.h>
typedef const __FlashStringHelper * FlashStr;
typedef const byte* PGM_BYTES_P;
#define PSTR_TO_F(s) reinterpret_cast<const __FlashStringHelper *> (s)

#include <PS2X_lib.h>

/******************************************************************
 * set pins connected to PS2 controller:
 *   - 1e column: original
 *   - 2e colmun: Stef?
 * replace pin numbers by the ones you use
 ******************************************************************/
#define PS2_SEL        10
#define PS2_CMD        11
#define PS2_CLK        12
#define PS2_DAT        13

PS2X<PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT> ps2x; // create PS2 Controller Class

boolean haveController = false;


const byte PSX_BUTTONS_NO = 16;

const char buttonSelectName[] PROGMEM = "Select";
const char buttonL3Name[] PROGMEM = "L3";
const char buttonR3Name[] PROGMEM = "R3";
const char buttonStartName[] PROGMEM = "Start";
const char buttonUpName[] PROGMEM = "Up";
const char buttonRightName[] PROGMEM = "Right";
const char buttonDownName[] PROGMEM = "Down";
const char buttonLeftName[] PROGMEM = "Left";
const char buttonL2Name[] PROGMEM = "L2";
const char buttonR2Name[] PROGMEM = "R2";
const char buttonL1Name[] PROGMEM = "L1";
const char buttonR1Name[] PROGMEM = "R1";
const char buttonTriangleName[] PROGMEM = "Triangle";
const char buttonCircleName[] PROGMEM = "Circle";
const char buttonCrossName[] PROGMEM = "Cross";
const char buttonSquareName[] PROGMEM = "Square";

const char* const psxButtonNames[PSX_BUTTONS_NO] PROGMEM = {
  buttonSelectName,
  buttonL3Name,
  buttonR3Name,
  buttonStartName,
  buttonUpName,
  buttonRightName,
  buttonDownName,
  buttonLeftName,
  buttonL2Name,
  buttonR2Name,
  buttonL1Name,
  buttonR1Name,
  buttonTriangleName,
  buttonCircleName,
  buttonCrossName,
  buttonSquareName
};

boolean initController () {
  boolean ret = false;

  switch (ps2x.begin ()) {
  case 0:
    // Controller is ready
	ret = true;
    break;
  case 1:
    Serial.println (F("No controller found"));
    break;
  case 2:
  default:
    Serial.println (F("Cannot initialize controller"));
    break;
  }

  return ret;
}

void printControllerType () {
	switch (ps2x.readType ()) {
    case 0:
      /* The Dual Shock controller gets recognized as this sometimes, or
       * anyway, whatever controller it is, it might work
       */
      Serial.println (F("Unknown Controller type found"));
      break;
    case 1:
      Serial.println (F("DualShock Controller found"));
      break;
    case 2:
      /* We used to refuse this, as it does not look suitable, but then we
       * found out that the Analog Controller (SCPH-1200) gets detected as
       * this... :/
       */
      Serial.println (F("Analog/GuitarHero Controller found"));
      break;
    case 3:
      Serial.println (F("Wireless Sony DualShock Controller found"));
      break;
    }
}

byte psxButtonToIndex (PsxButtons psxButtons) {
  byte i;

  for (i = 0; i < PSX_BUTTONS_NO; ++i) {
    if (psxButtons & 0x01) {
      break;
    }

    psxButtons >>= 1U;
  }

  return i;
}

FlashStr getButtonName (PsxButtons psxButton) {
  FlashStr ret = F("");
  
  byte b = psxButtonToIndex (psxButton);
  if (b < PSX_BUTTONS_NO) {
    PGM_BYTES_P bName = reinterpret_cast<PGM_BYTES_P> (pgm_read_ptr (&(psxButtonNames[b])));
    ret = PSTR_TO_F (bName);
  }

  return ret;
}

void dumpButtons (PsxButtons psxButtons) {
  static PsxButtons lastB = 0;

  if (psxButtons != lastB) {
    lastB = psxButtons;     // Save it before we alter it
    
    Serial.print (F("Pressed: "));

    for (byte i = 0; i < PSX_BUTTONS_NO; ++i) {
      byte b = psxButtonToIndex (psxButtons);
      if (b < PSX_BUTTONS_NO) {
        PGM_BYTES_P bName = reinterpret_cast<PGM_BYTES_P> (pgm_read_ptr (&(psxButtonNames[b])));
        Serial.print (PSTR_TO_F (bName));
      }

      psxButtons &= ~(1 << b);

      if (psxButtons != 0) {
        Serial.print (F(", "));
      }
    }

    Serial.println ();
  }
}

void setup () {
	Serial.begin (115200);

	delay (300); // Give wireless ps2 module some time to startup
}

void loop () {
	if (!haveController) {
		if (initController ()) {
			printControllerType ();
			haveController = true;
		}
	} else if (ps2x.read ()) {
		PsxButtons btns = ps2x.buttonWord ();
		dumpButtons (btns);
	} else {
		haveController = false;
	}
}
