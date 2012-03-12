/*
 Centurion - Bluetooth serial interface between a toaster and TouchPad
 V1.0 1/26/2012
 V1.1 1/30/2012
 V1.2 2/7/2012

*/

/* Defines */

/* Arduino pins */
#define pinButtons 0	// Analog in
#define pinDarkness 1	// Analog in
#define pinKeepWarm 6 // Digital out
#define pinBagel 7 // Digital out
#define pinDefrost 8 // Digital out
#define pinReheat 9 // Digital out
#define pinToastCancel 10 // Digital out
#define pinToasting 11	// Digital in
#define pinReset 12 // Digital out
#define pinLarson 13 // Digital out

/* Command characters sent and received over Bluetooth */
#define charKeepWarm 'k'
#define charBagel 'b'
#define charDefrost 'd'
#define charReheat 'r'
#define charToastCancel 't'
#define charReset 'x'
#define charLight '1'
#define charDark '7'
/* Command characters sent over Bluetooth */
#define charNone '.'
#define charUp '-'	// Toast up
#define charDown '+'	// Toast down
/* Command characters received over Bluetooth */
#define charQuery '?'	// Send toasting status charUp or charDown
#define charQueryShade 's' // Send shade level charLight to charDark


/* Constants */

/* Arduino outputs four bits to set toast shade */
const int darknessLevel[7] = {0xf, 0xd, 0xa, 0x8, 0x5, 0x2, 0x0};

/* Global variables */
unsigned long lastchangetime;
int now;
int prev;
int newtoast;
int toast;
char currentButton;
char currentDarkness;

/* Setup */
void setup() {
  // initialize serial:
  Serial.begin(115200);

	// Set data direction to output for shade level bits
	DDRD |= B00111100;
	// Do an initial read of shade knob to set toast shade
	setDarkness(getDarkness(true) - charLight);

	// Initialize buttons
  allInput();
	getButton(true);

	// Initialize output pin for Larson scanner (Cylon eye)
  pinMode(pinLarson, OUTPUT);

  // Initialize toasting input (pinToasting pulses while toasting)
  pinMode(pinToasting, INPUT);
  lastchangetime = millis();
  toast = LOW;
  newtoast = LOW;
  prev = digitalRead(pinToasting);

	// Send a reset command over Bluetooth
  Serial.write(charReset);
}

/* Test if a number falls within a range centered on target */
boolean analogCompare(int value, int target, int error) {
	if (value >= (target - error) && value <= (target + error)) {
		return true;
	}

	return false;
}

/* Read the buttons on the control panel */
char getButton(boolean init) {
	int value;
	char button;
	const int error = 20;

	value = analogRead(pinButtons);

	if (analogCompare(value, 200, error )) {
		button = charToastCancel;
	} else if (analogCompare(value, 349, error)) {
		button = charKeepWarm;
	} else if (analogCompare(value, 531, error)) {
		button = charBagel;
	} else if (analogCompare(value, 700, error)) {
		button = charDefrost;
	} else if (analogCompare(value, 1023, error)) {
		button = charReheat;
	} else {
		button = charNone;
	}

	if (init) {
		currentButton = button;
		return currentButton;
	} else {
		if (currentButton != button) {
			currentButton = button;
			return currentButton;
		} else {
			return charNone;
		}
	}
}

/* Change the toaster status based on a command corresponding to the buttons on the panel */
void setButton(char inChar) {
	switch (inChar) {
		case charReheat :
			outPin(pinReheat);
			break;
		case charDefrost :
			outPin(pinDefrost);
			break;
		case charBagel :
			outPin(pinBagel);
			break;
		case charKeepWarm :
			outPin(pinKeepWarm);
			break;
		case charToastCancel :
			outPin(pinToastCancel);
			break;
	}
}

/* Read the toast shade knob */
char getDarkness(boolean init) {
	int value;
	char darkness;
	const int error = 40;

	value = analogRead(pinDarkness);

	if (value <= 80 ) {
		darkness = '7';
	} else if (value <= 180) {
		darkness = '6';
	} else if (value <= 280) {
		darkness = '5';
	} else if (value <= 380) {
		darkness = '4';
	} else if (value <= 550) {
		darkness = '3';
	} else if (value <= 750) {
		darkness = '2';
	} else {
		darkness = '1';
	}

	if (init) {
		currentDarkness = darkness;
		return currentDarkness;
	} else {
		if (currentDarkness != darkness) {
			currentDarkness = darkness;
			return currentDarkness;
		} else {
			return charNone;
		}
	}
}

/* Set  the toast shade */
void setDarkness(int darkness) {
	if (darkness >= 0 && darkness <= 6) {
		PORTD = (PORTD & B11000011) | (darknessLevel[darkness] << 2);
	}
}

/* Set all 'buttons' to INPUT fro high impedance */
void allInput() {
  pinMode(pinToastCancel, INPUT);
  pinMode(pinKeepWarm, INPUT);
  pinMode(pinBagel, INPUT);
  pinMode(pinDefrost, INPUT);
  pinMode(pinReheat, INPUT);
}

/* Pulse a button */
void outPin(int pi) {
  pinMode(pi, OUTPUT);     
  digitalWrite(pi, HIGH);
  delay(500);
  digitalWrite(pi, LOW);
  pinMode(pi, INPUT);     
}

/* Send a character via Bluetooth indicating the toasting status
   '+' = Toasing
   '-' = Not toasting
*/
void sendToast() {
	char toastChar;

	if (toast == HIGH) {
		toastChar = '+';
	} else {
		toastChar = '-';
	}
    Serial.write(toastChar);
}

/* Send the toast shade level over Bluetooth */
void sendShade() {
	Serial.write(getDarkness(true));
}

/* main loop */
void loop() {
	char panel;

	// read panel
	panel = getDarkness(false);
	if (panel != charNone) {
		setDarkness(panel - charLight);
		Serial.write(panel);
	}

	panel = getButton(false);
	if (panel != charNone) {
		setButton(panel);
		Serial.write(panel);
	}

	// test toasting state
  now = digitalRead(pinToasting);
  if (now != prev)
  {
		prev = now;
		lastchangetime = millis();
  }
    
  if (millis() - lastchangetime < 100)
  {
		newtoast = HIGH;
  } else {
		newtoast = LOW;
  }

  if (newtoast != toast)
  {
		toast = newtoast;
    digitalWrite(pinLarson, toast);
    sendToast();
  }
}

/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */
void serialEvent() {
  while (Serial.available()) {
    allInput();
    // get the new byte:
    char inChar = (char)Serial.read();

    switch (inChar) {
      case charLight ... charDark :
        setDarkness(inChar-charLight);
        break;
      case charReheat :
        outPin(pinReheat);
        break;
      case charDefrost :
        outPin(pinDefrost);
        break;
      case charBagel :
        outPin(pinBagel);
        break;
      case charKeepWarm :
        outPin(pinKeepWarm);
        break;
      case charToastCancel :
        outPin(pinToastCancel);
        break;
      case charReset :
        outPin(pinReset);
        break;
      case charQuery :
        sendToast();
        break;
      case charQueryShade :
        sendShade();
        break;
    }
  }
}


