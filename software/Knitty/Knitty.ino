//////////////////////////////////////////////////////////////////////////////
// Knitty Project
//
// Author: ptflea, schinken
//

//Servo
#include <Servo.h>

Servo servoColour12;  // create servo object to control a servo
Servo servoColour34;

#define INT_ENCODER 0


//////////////////////////////////////////////////////////////////////////////
// General purpose definitions

//Pin 3 unused
#define PIN_CSENSE    2         // Yellow
#define PIN_CREF      4         // White
#define PIN_NEEDLE_RTL    5         // Blue,  Pattern RTL
#define PIN_NEEDLE_LTR    6         // ,  Pattern LTR
#define PIN_BUTTON_1  7         // Button_1 (activate colour change)
#define BUTTONDELAY   20        // delay for Button_1
// PIN 8 and 9 are for the color change servos
#define PIN_Eyelet_1  10        // Eyelet_1 status
#define Eyelet_1_DELAY   100
#define PIN_Eyelet_2  11        // Eyelet_2 status
#define PIN_Eyelet_3  12        // Eyelet_3 status
#define PIN_Eyelet_4  13        // Eyelet_4 status


long buttonLastChecked = 0; // variable to limit the button getting checked every cycle
int button_1_State = 0;     // status of button_1
int button_1_Hold = 0; // toggle state of Button_1

long eyelet_1_LastChecked = 0;
int eyelet_1_State = 0;
int eyelet_1_Hold = 0;

#define DIRECTION_UNKNOWN       0
#define DIRECTION_LEFT_RIGHT   -1
#define DIRECTION_RIGHT_LEFT    1

char currentDirection = DIRECTION_UNKNOWN;
char lastDirection = DIRECTION_UNKNOWN;

signed int currentCursorPosition = 0;
unsigned int currentPatternIndex = 0;
signed int firstNeedle = 0;
signed int offsetCarriage_RTL = 52;
signed int offsetCarriage_LTR = 30;


volatile unsigned char knitPattern[255] = {
  0};
bool isKnitting = false;

volatile unsigned char passapCalibrateArray[8] = { 0 };
signed int  passapCalibratePointer = 0;
static unsigned char passaptestpatter[8] = {1, 1, 0, 1, 1, 0, 0, 0};


//////////////////////////////////////////////////////////////////////////////
// Knitty Serial Protocol

// Receive commands
#define COM_CMD_PATTERN      'P'
#define COM_CMD_PATTERN_END  'E'
#define COM_CMD_CURSOR       'C'
#define COM_CMD_RESPONSE     'R'
#define COM_CMD_DIRECTION    'D'
#define COM_CMD_DEBUG        'V'
#define COM_CMD_NEW_PATTERN  'N'
#define COM_CMD_FIRST_NEEDLE 'F'  //first needle of pattern from right
#define COM_CMD_SEPERATOR    ':'

#define COM_CMD_SERVO        'S'

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

  // Button Input
  pinMode(PIN_BUTTON_1, INPUT);

  //Eylet Input
  pinMode(PIN_Eyelet_1, INPUT);


  // Setup PHOTO SENSOR pin change interrupt
  pinMode(PIN_CSENSE, INPUT);
  pinMode(PIN_CREF, INPUT);
  attachInterrupt(INT_ENCODER, interruptPinChangeEncoder, CHANGE);

  // Setup Needles
  pinMode(PIN_NEEDLE_RTL, OUTPUT);
  digitalWrite(PIN_NEEDLE_RTL, HIGH);
  pinMode(PIN_NEEDLE_LTR, OUTPUT);
  digitalWrite(PIN_NEEDLE_LTR, HIGH);

}

void executeCommand(unsigned char cmd, String payload) {

  switch(cmd) {
  case COM_CMD_PATTERN:
    Serial.print("P ");
    Serial.println(patternLength);
    break;

  case COM_CMD_CURSOR:
    currentCursorPosition = payload.toInt();
    break;

  case COM_CMD_FIRST_NEEDLE:
    firstNeedle = payload.toInt()*2-2;
    sendCommand(COM_CMD_RESPONSE, String(firstNeedle));
    break;

  case COM_CMD_SERVO:

    switch(payload.toInt()) {
    case 0: //no colour selected
      servoColour12.write(90);
      servoColour34.write(90);
      break;
    case 1: //Color 1
      servoColour12.write(70);
      servoColour34.write(90);
      break;
    case 2: //Color 2
      servoColour12.write(115);
      servoColour34.write(90);
      break;
    case 3: //Color 3
      servoColour12.write(90);
      servoColour34.write(70);
      break;
    case 4: //Color 4
      servoColour12.write(90);
      servoColour34.write(115);
      break;
    case 5: //Servo off
      servoColour12.detach();
      servoColour34.detach();
      break;
    case 6: //Servo on
      servoColour12.attach(8);
      servoColour34.attach(9);
      break;
    }

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
    if (buffer == COM_CMD_PATTERN) {
     patternLength = 0;
    }
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

      sendCommand(COM_CMD_RESPONSE, "P OK");
      break;
    }

    if (parserReceivedCommand == COM_CMD_PATTERN) {
        knitPattern[patternLength] = (buffer == '1')? 1 : 0;
        patternLength += 1;
    } else {
        parserReceivedPayload += buffer;
    }
    break;
  }
}

void loop() {
  parserSerialStream();

  //check if button for colour change ist activated and send response to processing
  button_1_State = digitalRead(PIN_BUTTON_1);

  if( buttonLastChecked == 0 ) // see if this is the first time checking the buttons
    buttonLastChecked = millis()+BUTTONDELAY;  // force a check this cycle
  if( millis() - buttonLastChecked > BUTTONDELAY ) { // make sure a reasonable delay passed
    if (button_1_State == HIGH) {
      if (button_1_Hold == HIGH) {
        // Send
        sendCommand(COM_CMD_RESPONSE, "ON");
        button_1_Hold = LOW;
      }
    } else if (button_1_Hold == LOW) {
        // Send
        sendCommand(COM_CMD_RESPONSE, "OFF");
        button_1_Hold = HIGH;
    }
    buttonLastChecked = millis(); // reset the lastChecked value
  }



//  //check Eyelets
//  eyelet_1_State = digitalRead(PIN_Eyelet_1);
//
//  if( eyelet_1_LastChecked == 0 ) // see if this is the first time checking the buttons
//    eyelet_1_LastChecked = millis()+Eyelet_1_DELAY;  // force a check this cycle
//  if( millis() - eyelet_1_LastChecked > Eyelet_1_DELAY ) { // make sure a reasonable delay passed
//    if (eyelet_1_State == HIGH) {
//      if (eyelet_1_Hold == HIGH) {
//        // Send
//        sendCommand(COM_CMD_RESPONSE, "Eyelet 1 OUT");
//        eyelet_1_Hold = LOW;
//      }
//    }
//    else {
//      if (eyelet_1_Hold == LOW) {
//        // Send
//        sendCommand(COM_CMD_RESPONSE, "Eyelet 1 IN");
//        eyelet_1_Hold = HIGH;
//      }
//    }
//    eyelet_1_LastChecked = millis(); // reset the lastChecked value
//  }

}




void setNeedleByCursor(int cursorPosition) {

  // Just to be sure that we never exceed the pattern
  //  if(cursorPosition > patternLength-1 || cursorPosition < 0) {
  //    return;
  //  }

  if(currentDirection == DIRECTION_LEFT_RIGHT) {
    setNeedle_LTR(knitPattern[cursorPosition]);
  }
  else if(currentDirection == DIRECTION_RIGHT_LEFT) {
    setNeedle_RTL(knitPattern[patternLength-cursorPosition-1]);
  }
}



void setNeedle_RTL(char state) {
  //change state because the E6000 sets needle by 0
  if(state==1){
    state = 0;
  } else {
    state = 1;
  }
  digitalWrite(PIN_NEEDLE_RTL, state);
}

void setNeedle_LTR(char state) {
  //change state because the E6000 sets needle by 0
  if(state==1){
    state = 0;
  }
  else
  {
    state = 1;
  }
  digitalWrite(PIN_NEEDLE_LTR, state);
}


void interruptPinChangeEncoder() {

  char currentPinChangeValue = digitalRead(PIN_CSENSE);
  char currentPinChangeOppositeValue = digitalRead(PIN_CREF);

  //  Serial.print(String(0+currentPinChangeValue));
  //  Serial.print("-");
  //  Serial.print(String(0+currentPinChangeOppositeValue));
  //  Serial.print("-");
  //  Serial.println(String(currentCursorPosition));

  // Determine direction
  if(currentPinChangeValue == currentPinChangeOppositeValue) {
    currentDirection = DIRECTION_LEFT_RIGHT;
  } else {
    currentDirection = DIRECTION_RIGHT_LEFT;
  }

  // RTL = 1, LTR = -1
  currentCursorPosition += currentDirection; 




  //debug cursorposition
  //  if(currentCursorPosition==0){
  //  sendCommand(COM_CMD_RESPONSE, String(currentCursorPosition));
  ////sendCommand(COM_CMD_RESPONSE, String(firstNeedle));
  //  }

  // Check if we're in range of our needles
  if (currentCursorPosition >=-4){
    if((currentDirection == DIRECTION_RIGHT_LEFT && currentCursorPosition > offsetCarriage_RTL + firstNeedle) ||
      (currentDirection == DIRECTION_LEFT_RIGHT && currentCursorPosition - offsetCarriage_LTR  <= patternLength*2 + firstNeedle)) {
      //sendCommand(COM_CMD_RESPONSE, String(currentPatternIndex));
      if(currentPatternIndex > patternLength) {

        setNeedle_RTL(0);
        setNeedle_LTR(0);
        currentPatternIndex = 0;
        isKnitting = false;

        sendCommand(COM_CMD_PATTERN_END, "1");


      } else if(isKnitting == true) {
          // React on FALLING Edge
          if(currentPinChangeValue == 1)  {
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
      //sendCommand(COM_CMD_DIRECTION, "RTL");
    } 
    else {
      // sendCommand(COM_CMD_DIRECTION, "LTR");
    }
  }

  //AutoCalibrate
  passapCalibrateArray[passapCalibratePointer & 0x07] = currentPinChangeValue;
  passapCalibrateArray[(passapCalibratePointer+1) & 0x07] = currentPinChangeOppositeValue;

  if (passapCalibratePointer > 8){ // 16
    // read last 16 digits of passapCalibrateArray
    int found = 1;
    for (int i=0; i<8; i++){
      if (passapCalibrateArray[(passapCalibratePointer-i+2) & 0x07] !=
          passaptestpatter[i]) {
        found = 0;
        break;
      }
    }
    if (found){
      Serial.println("Starting new line");
      //calibrate
      currentCursorPosition = 0;
      passapCalibratePointer = 0;
    }
  }
  passapCalibratePointer = passapCalibratePointer +2;
}
