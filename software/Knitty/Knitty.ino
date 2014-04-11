//////////////////////////////////////////////////////////////////////////////
// Knitty Project
//
// Author: ptflea, schinken
//

#define INT_ENCODER 0
#define INT_IFDR    1

//////////////////////////////////////////////////////////////////////////////
// General purpose definitions

#define PIN_IFDR      3         // Green
#define PIN_CSENSE    2         // Yellow
#define PIN_CREF      4         // White
#define PIN_NEEDLE    5         // Blue,  Pattern

#define DIRECTION_UNKNOWN       0
#define DIRECTION_LEFT_RIGHT   -1
#define DIRECTION_RIGHT_LEFT    1

char currentDirection = DIRECTION_UNKNOWN;
char lastDirection = DIRECTION_UNKNOWN;

signed int currentCursorPosition = -15;
signed int leftEndCursorPosition = 0;
unsigned int currentPatternIndex = 0;

volatile unsigned char knitPattern[255] = {0};
bool isKnitting = false;

//////////////////////////////////////////////////////////////////////////////
// Knitty Serial Protocol

// Receive commands
#define COM_CMD_PATTERN      'P'
#define COM_CMD_PATTERN_END  'E'
#define COM_CMD_CURSOR       'C'
#define COM_CMD_IFDR         'I'
#define COM_CMD_RESPONSE     'R'
#define COM_CMD_DIRECTION    'D'
#define COM_CMD_DEBUG        'V'
#define COM_CMD_NEW_PATTERN  'N'
#define COM_CMD_SEPERATOR    ':'

#define COM_CMD_PLOAD_END    '\n'

// Parser states
#define COM_PARSE_CMD      0x01
#define COM_PARSE_SEP      0x02
#define COM_PARSE_PLOAD    0x03


unsigned char parserState = COM_PARSE_CMD;

unsigned char parserReceivedCommand = 0;
String parserReceivedPayload = "";

unsigned char patternLength = 0;

void setup() {
  Serial.begin(115200);

  // Setup PHOTO SENSOR pin change interrupt
  pinMode(PIN_CSENSE, INPUT);
  pinMode(PIN_CREF, INPUT);
  attachInterrupt(INT_ENCODER, interruptPinChangeEncoder, CHANGE);

  // Setup IFDR pin change interrupt
  pinMode(PIN_IFDR, INPUT);
  attachInterrupt(INT_IFDR, interruptPinChangeIfdr, FALLING);

  // Setup Needle
  pinMode(PIN_NEEDLE, OUTPUT);
  digitalWrite(PIN_NEEDLE, LOW);

}

void executeCommand(unsigned char cmd, String payload) {

  switch(cmd) {
    case COM_CMD_PATTERN:

      patternLength = payload.length();
      for(unsigned char i = 0; i < patternLength; i++) {
        knitPattern[i] = (payload.charAt(i) == '1')? 1 : 0;
      }

      break;

    case COM_CMD_CURSOR:
      currentCursorPosition = payload.toInt();
      break;
  }
}

void sendCommand(unsigned char cmd, String payload) {
  Serial.write(cmd);
  Serial.write(COM_CMD_SEPERATOR);
  Serial.print(payload);
  Serial.write("\n");
}

void parserSerialStream() {

  if (Serial.available() == 0) {
    return;
  }

  char buffer = Serial.read();

  switch(parserState) {

    case COM_PARSE_CMD:
      parserState = COM_PARSE_SEP;
      parserReceivedCommand = buffer;
      parserReceivedPayload = "";
      break;

    case COM_PARSE_SEP:

      // We're awaiting a seperator here, if not, back to cmd
      if(buffer == COM_CMD_SEPERATOR) {
        parserState = COM_PARSE_PLOAD;
        break;
      }

      parserState = COM_PARSE_CMD;
      break;

    case COM_PARSE_PLOAD:

      if(buffer == COM_CMD_PLOAD_END) {

        executeCommand(parserReceivedCommand, parserReceivedPayload);
        parserState = COM_PARSE_CMD;

        sendCommand(COM_CMD_RESPONSE, "OK");
        break;
      }

      parserReceivedPayload += buffer;
      break;
  }
}

void loop() {
  parserSerialStream();
}

void setNeedleByCursor(char cursorPosition) {

  // Just to be sure that we never exceed the pattern
  if(cursorPosition > patternLength-1 || cursorPosition < 0) {
    return;
  }

  if(currentDirection == DIRECTION_LEFT_RIGHT) {
    setNeedle(knitPattern[cursorPosition]);
  } else if(currentDirection == DIRECTION_RIGHT_LEFT) {
    setNeedle(knitPattern[patternLength-cursorPosition-1]);
  }
}

void setNeedle(char state) {
  digitalWrite(PIN_NEEDLE, state);
}

void interruptPinChangeEncoder() {

  char currentPinChangeValue = digitalRead(PIN_CSENSE);
  char currentPinChangeOppositeValue = digitalRead(PIN_CREF);

  // Determine direction
  if(currentPinChangeValue == currentPinChangeOppositeValue) {
    currentDirection = DIRECTION_LEFT_RIGHT;
  } else {
    currentDirection = DIRECTION_RIGHT_LEFT;
  }

  // RTL = 1, LTR = -1
  currentCursorPosition += currentDirection; 

  // Check if we're in range of our needles
  if((currentDirection == DIRECTION_RIGHT_LEFT && currentCursorPosition > 0) ||
    (currentDirection == DIRECTION_LEFT_RIGHT && currentCursorPosition <= leftEndCursorPosition)) {

    if(currentPatternIndex > patternLength) {

      setNeedle(0);
      currentPatternIndex = 0;
      isKnitting = false;

      sendCommand(COM_CMD_PATTERN_END, "1");

      // Remember last cursor position to begin for the opposite direction
      if(currentDirection == DIRECTION_RIGHT_LEFT) {
        //cursor position differs from RTL to LTR
        leftEndCursorPosition = currentCursorPosition-5;
      }

    } else {

      if(isKnitting == true) {
        // React on FALLING Edge for RTL, RISING for LTR
        if((currentDirection == DIRECTION_RIGHT_LEFT && currentPinChangeValue == 0) ||
           (currentDirection == DIRECTION_LEFT_RIGHT && currentPinChangeValue == 1) ) {
          setNeedleByCursor(currentPatternIndex);
          currentPatternIndex++;
        }
      }

    }
  }

  if(lastDirection != currentDirection) {
    lastDirection = currentDirection;
    currentPatternIndex = 0;
    isKnitting = true;
    if(currentDirection == DIRECTION_RIGHT_LEFT) {
      sendCommand(COM_CMD_DIRECTION, "RTL");
    } 
    else {
      sendCommand(COM_CMD_DIRECTION, "LTR");
    }
  }
}

// We only use the IFDR to determine if we can send the pattern
// for the next line.
void interruptPinChangeIfdr() {
  if(isKnitting == false) {
    sendCommand(COM_CMD_IFDR, "1");
  }
}


