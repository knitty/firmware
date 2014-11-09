

//////////////////////////////////////////////////////////////////////////////
// Knitty Project
//
// Author: ptflea, schinken, 
//

//Servo
#include <Servo.h>

Servo servoColour12;  // create servo object to control a servo
Servo servoColour34;

//define interupts for CSENSE
#define INT_ENCODER 0   
#define INT_ENCODER_BACK 1


//////////////////////////////////////////////////////////////////////////////
// General purpose definitions

//front carriage
#define PIN_CSENSE  2         // Yellow
#define PIN_CREF    4         // White
#define PIN_NEEDLE_RTL 5      // Blue,  Pattern RTL
#define PIN_NEEDLE_LTR 6      // ,  Pattern LTR
//back carriage
#define PIN_CSENSE_BACK   3         // 
#define PIN_CREF_BACK    12         // 
#define PIN_NEEDLE_RTL_BACK 11      //   Pattern RTL
#define PIN_NEEDLE_LTR_BACK 10      //   Pattern LTR


#define PIN_BUTTON_1 13// 7         // Button_1 (activate colour change)
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

#define DIRECTION_UNKNOWN_BACK       0
#define DIRECTION_LEFT_RIGHT_BACK   -1
#define DIRECTION_RIGHT_LEFT_BACK    1


char currentDirection = DIRECTION_UNKNOWN;
char lastDirection = DIRECTION_UNKNOWN;
char currentDirection_back = DIRECTION_UNKNOWN;
char lastDirection_back = DIRECTION_UNKNOWN;

//signed int reactOnEdge_back = 1; 

signed int currentCursorPosition = 0;
unsigned long lastCursorChange = 0;
unsigned int currentPatternIndex = 0;
signed int firstNeedle = 0;
signed int offsetCarriage_RTL = 52;
signed int offsetCarriage_LTR = 30;

signed int currentCursorPosition_back = 0;
unsigned long lastCursorChange_back = 0;
unsigned int currentPatternIndex_back = 0;
signed int firstNeedle_back = 0;
signed int offsetCarriage_RTL_back = 52;
signed int offsetCarriage_LTR_back = 31;

volatile unsigned char knitPattern[255] = { 
  0 };
volatile unsigned char knitPattern_back[255] = { 
  0 };

bool isKnitting = false;

volatile unsigned char passapCalibrateArray[8] = { 
  0 };
signed int  passapCalibratePointer = 0;
static unsigned char passaptestpattern[8] = { 1, 1, 0, 1, 1, 0, 0, 0};

volatile unsigned char passapCalibrateArray_back[8] = { 
  0 };
signed int  passapCalibratePointer_back = 0;
//static unsigned char passaptestpattern_back[8] = { 1, 1, 0, 1, 1, 0, 0, 0};
static unsigned char passaptestpattern_back[8] = { 1, 0, 0, 1, 1, 1, 0, 1};
//static unsigned char passaptestpattern_back[8] = {  0, 0, 1, 1, 0, 1, 1, 1};



//////////////////////////////////////////////////////////////////////////////
// Knitty Serial Protocol

// Receive commands
#define COM_CMD_PATTERN      'P'
#define COM_CMD_PATTERN_BACK 'B'
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
unsigned char patternLengthBack = 0;

void setup() {
  Serial.begin(115200);
  sendCommand(COM_CMD_RESPONSE, "up and running");

  // Button Input
  pinMode(PIN_BUTTON_1, INPUT);

  //Eylet Input
  pinMode(PIN_Eyelet_1, INPUT);


  // Setup PHOTO SENSOR pin change interrupt
  pinMode(PIN_CSENSE, INPUT_PULLUP);
  pinMode(PIN_CSENSE_BACK, INPUT_PULLUP);

  pinMode(PIN_CREF,  INPUT_PULLUP);
  pinMode(PIN_CREF_BACK,  INPUT_PULLUP);

  attachInterrupt(INT_ENCODER, interruptPinChangeEncoder, CHANGE);
  attachInterrupt(INT_ENCODER_BACK, interruptPinChangeEncoder_back, CHANGE);

  // Setup Needles
  pinMode(PIN_NEEDLE_RTL, OUTPUT);
  digitalWrite(PIN_NEEDLE_RTL, HIGH);
  pinMode(PIN_NEEDLE_LTR, OUTPUT);
  digitalWrite(PIN_NEEDLE_LTR, HIGH);

  pinMode(PIN_NEEDLE_RTL_BACK, OUTPUT);
  digitalWrite(PIN_NEEDLE_RTL_BACK, HIGH);
  pinMode(PIN_NEEDLE_LTR_BACK, OUTPUT);
  digitalWrite(PIN_NEEDLE_LTR_BACK, HIGH);
}

void executeCommand(unsigned char cmd, String payload) {

  switch(cmd) {
  case COM_CMD_PATTERN:
    //Serial.print("P ");
    //Serial.println(patternLength);
   // sendCommand(COM_CMD_RESPONSE, "PatternLength: " + String(patternLength));
    break;

  case COM_CMD_PATTERN_BACK:
    //Serial.print("B ");
    //Serial.println(patternLengthBack);
    //sendCommand(COM_CMD_RESPONSE, "PatternLengthBack: " + String(patternLength));
    break;

  case COM_CMD_CURSOR:
    currentCursorPosition = payload.toInt();
    break;

  case COM_CMD_FIRST_NEEDLE:
    firstNeedle = payload.toInt()*2-2;
    
    sendCommand(COM_CMD_RESPONSE, "FirstNeedle: " + String(firstNeedle));
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
    
    if (buffer == COM_CMD_PATTERN_BACK) {
      patternLengthBack = 0;
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

      sendCommand(COM_CMD_RESPONSE, "Recieved");
      break;
    }

    if (parserReceivedCommand == COM_CMD_PATTERN) {
      //Change state because the E6000 set the needle at '0'
      knitPattern[patternLength] = (buffer == '0')? 1 : 0;
      //reverse pattern
      //knitPattern_back[patternLength] = (buffer == '1')? 1 : 0;
             
     // Serial.println(String(knitPattern_back[patternLength])+String(patternLength));
      patternLength += 1;
    } 
    else if (parserReceivedCommand == COM_CMD_PATTERN_BACK) {
      //Change state because the E6000 set the needle at '0'
      knitPattern_back[patternLengthBack] = (buffer == '0')? 1 : 0;
     //Serial.println(String(knitPattern_back[patternLength])+String(patternLength));
      patternLengthBack += 1;
    } 
    else {
      parserReceivedPayload += buffer;
    }
    
    break;
  }
}


void loop() {
  parserSerialStream();
}


void setNeedleByCursor(int currentPatternIndexSet) {

  if(currentDirection == DIRECTION_LEFT_RIGHT) {
        if (patternLength <= currentPatternIndexSet){
         setNeedle_LTR(1);
       //  Serial.println("StateLTR:lastNeedle");
        }
         else {
      //  Serial.println("StateLTR:"+String(knitPattern_back[currentPatternIndexSet_back])+"-"+String(currentPatternIndexSet_back));
        setNeedle_LTR(knitPattern[currentPatternIndexSet]);
         }
  }
  else if(currentDirection == DIRECTION_RIGHT_LEFT) {
        if (patternLength-currentPatternIndexSet-1 < 0){
              setNeedle_RTL(1);
        // Serial.println("StateRTL:lastNeedle");
        }
       else{ 
       // Serial.println("StateRTL:"+String(knitPattern_back[patternLength-currentPatternIndexSet_back-1])+"-"+String(patternLength-currentPatternIndexSet_back-1));
        setNeedle_RTL(knitPattern[patternLength-currentPatternIndexSet-1]);
       }
  }
//  if(currentDirection == DIRECTION_LEFT_RIGHT) {
//    setNeedle_LTR(knitPattern[currentPatternIndexSet]);
//  }
//  else if(currentDirection == DIRECTION_RIGHT_LEFT) {
//    setNeedle_RTL(knitPattern[patternLength-currentPatternIndexSet-1]);
//  }
}

void setNeedleByCursor_back(int currentPatternIndexSet_back) {

  // Just to be sure that we never exceed the pattern
  //  if(cursorPosition > patternLength-1 || cursorPosition < 0) {
  //    return;
  //  }

  if(currentDirection_back == DIRECTION_LEFT_RIGHT) {
        if (patternLength <= currentPatternIndexSet_back){
         setNeedle_LTR_back(1);
       //  Serial.println("StateLTR:lastNeedle");
        }
         else {
      //  Serial.println("StateLTR:"+String(knitPattern_back[currentPatternIndexSet_back])+"-"+String(currentPatternIndexSet_back));
        setNeedle_LTR_back(knitPattern_back[currentPatternIndexSet_back]);
         }
  }
  else if(currentDirection_back == DIRECTION_RIGHT_LEFT) {
        if (patternLength-currentPatternIndexSet_back-1 < 0){
              setNeedle_RTL_back(1);
        // Serial.println("StateRTL:lastNeedle");
        }
       else{ 
       // Serial.println("StateRTL:"+String(knitPattern_back[patternLength-currentPatternIndexSet_back-1])+"-"+String(patternLength-currentPatternIndexSet_back-1));
        setNeedle_RTL_back(knitPattern_back[patternLength-currentPatternIndexSet_back-1]);
       }
  }
}


void setNeedle_RTL(char state) {
  //change state because the E6000 sets needle by 0
  //  Serial.print("-");
  //  Serial.print(String(0+state));
//  if(state==1){
//    state = 0;
//  } 
//  else {
//    state = 1;
//  }
  digitalWrite(PIN_NEEDLE_RTL, state);
}

void setNeedle_RTL_back(char state) {
  //change state because the E6000 sets needle by 0
//  if(state==1){
//    state = 0;
//  } 
//  else {
//    state = 1;
//  }
  digitalWrite(PIN_NEEDLE_RTL_BACK, state);
}

void setNeedle_LTR(char state) {
  //change state because the E6000 sets needle by 0
//  if(state==1){
//    state = 0;
//  }
//  else
//  {
//    state = 1;
//  }
  digitalWrite(PIN_NEEDLE_LTR, state);
}

void setNeedle_LTR_back(char state) {
  //change state because the E6000 sets needle by 0
//  if(state==1){
//    state = 0;
//  }
//  else
//  {
//    state = 1;
//  }
  digitalWrite(PIN_NEEDLE_LTR_BACK, state);
}


void interruptPinChangeEncoder() {

  unsigned long now = micros();
  if (now - lastCursorChange < 1000) {
    lastCursorChange = now;
    return;
  }
  lastCursorChange = now;

  char currentPinChangeValue = digitalRead(PIN_CSENSE);
  char currentPinChangeOppositeValue = digitalRead(PIN_CREF);

  //  Serial.print(String(0+currentPinChangeValue));
  //  Serial.print("-");
  //  Serial.println(String(0+currentPinChangeOppositeValue));

  // Determine direction
  if(currentPinChangeValue == currentPinChangeOppositeValue) {
    currentDirection = DIRECTION_LEFT_RIGHT;
  } 
  else {
    currentDirection = DIRECTION_RIGHT_LEFT;
  }

  // RTL = 1, LTR = -1
  currentCursorPosition += currentDirection; 

  // Serial.print(String(currentPatternIndex));
  // Serial.print("-");
  // Serial.print(String(currentCursorPosition));



  //debug cursorposition
  //  if(currentCursorPosition==0){
  //  sendCommand(COM_CMD_RESPONSE, String(currentCursorPosition));
  ////sendCommand(COM_CMD_RESPONSE, String(firstNeedle));
  //  }


  if (currentCursorPosition >20 && currentCursorPosition < 420  ){   // Check if we're in range of our needles
    if((currentDirection == DIRECTION_RIGHT_LEFT && currentCursorPosition > offsetCarriage_RTL + firstNeedle) ||
      (currentDirection == DIRECTION_LEFT_RIGHT && currentCursorPosition - offsetCarriage_LTR  <= patternLength*2 + firstNeedle)) {
      //sendCommand(COM_CMD_RESPONSE, String(currentPatternIndex));
      if(currentPatternIndex > patternLength) {

        //Set to '1' because the E6000 sets needle by 0
        setNeedle_RTL(1);
        setNeedle_LTR(1);
        currentPatternIndex = 0;
        isKnitting = false;
        
        sendCommand(COM_CMD_PATTERN_END, "1");
      } 
      else if(isKnitting == true) {
        //        Serial.print(String(1+currentDirection));
        //        Serial.print("-");
        //        Serial.print(String(currentCursorPosition));
        //        Serial.print("-");
        //        Serial.println(String(offsetCarriage_RTL + firstNeedle));


        // React on FALLING Edge     
        if(currentPinChangeValue == 1)  {
          setNeedleByCursor(currentPatternIndex);
          currentPatternIndex++;
        }
      }
    }
  }

  // Serial.println();

  if (currentCursorPosition >30 && currentCursorPosition < 420  ){ //don't check if not in needle range
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

  //AutoCalibrate
  passapCalibrateArray[passapCalibratePointer & 0x07] = currentPinChangeValue;
  passapCalibrateArray[(passapCalibratePointer+1) & 0x07] = currentPinChangeOppositeValue;

  if (passapCalibratePointer > 8){ // 16
    // read last 16 digits of passapCalibrateArray
    int found = 1;
    for (int i=0; i<8; i++){
      if (passapCalibrateArray[(passapCalibratePointer-i+2) & 0x07] !=
        passaptestpattern[i]) {
        found = 0;
        break;
      }
    }
    if (found){
      //sendCommand(COM_CMD_RESPONSE, "Calibrate");
      //calibrate
      currentCursorPosition = -2;
      passapCalibratePointer = 0;
    }
  }
  passapCalibratePointer = passapCalibratePointer +2;
}



void interruptPinChangeEncoder_back() {

  unsigned long now = micros();
  if (now - lastCursorChange_back < 1000) {
    lastCursorChange_back = now;
    return;
  }
  lastCursorChange_back = now;

  char currentPinChangeValue_back = digitalRead(PIN_CSENSE_BACK);
  char currentPinChangeOppositeValue_back = digitalRead(PIN_CREF_BACK);

//    Serial.print(String(0+currentPinChangeValue_back));
//    Serial.print("-");
//    Serial.println(String(0+currentPinChangeOppositeValue_back));

  // Determine direction
  if(currentPinChangeValue_back == currentPinChangeOppositeValue_back) {
  //    currentDirection_back = DIRECTION_LEFT_RIGHT;
  currentDirection_back = DIRECTION_RIGHT_LEFT;
  } 
  else {
//      currentDirection_back = DIRECTION_RIGHT_LEFT;
    currentDirection_back = DIRECTION_LEFT_RIGHT;
  }

  // RTL = 1, LTR = -1
  currentCursorPosition_back += currentDirection_back; 

//   Serial.print(String(currentPatternIndex_back));
//   Serial.print("-");
//   Serial.print(String(currentCursorPosition_back));



  //debug cursorposition
  //  if(currentCursorPosition==0){
  //  sendCommand(COM_CMD_RESPONSE, String(currentCursorPosition));
  ////sendCommand(COM_CMD_RESPONSE, String(firstNeedle));
  //  }


  if (currentCursorPosition_back >20 && currentCursorPosition_back < 420  ){   // Check if we're in range of our needles
    if((currentDirection_back == DIRECTION_RIGHT_LEFT && currentCursorPosition_back > offsetCarriage_RTL_back + firstNeedle) ||
      (currentDirection_back == DIRECTION_LEFT_RIGHT && currentCursorPosition_back - offsetCarriage_LTR_back  <= patternLength*2 + firstNeedle)) {
      //Serial.println("CPI"+String(currentPatternIndex_back));
      if(currentPatternIndex_back > patternLength) {

        //Set to '1' because the E6000 sets needle by 0
        setNeedle_RTL_back(1);
        setNeedle_LTR_back(1);
        currentPatternIndex_back = 0;
        //isKnitting = false;

       // sendCommand(COM_CMD_PATTERN_END, "1");
      } 
      else if(isKnitting == true) {
//                Serial.print(String(1+currentDirection_back));
//                Serial.print("-");
//                Serial.print(String(currentCursorPosition_back));
//                Serial.print("-");
//                Serial.println(String(offsetCarriage_RTL_back + firstNeedle));


//        // React on FALLING/RAISING edge depending on direction  
//       if (currentDirection_back == DIRECTION_RIGHT_LEFT){
//       reactOnEdge_back = 1;
//       }
//       else{
//       reactOnEdge_back = 0;
//       }
       
        
        if(currentPinChangeValue_back == 1)  {
          setNeedleByCursor_back(currentPatternIndex_back);
         // Serial.println("PI: "+String(currentPatternIndex_back));
          currentPatternIndex_back++;
        }
      }
    }
  }

  // Serial.println();

  if (currentCursorPosition_back >30 && currentCursorPosition_back < 420  ){ //don't check if not in needle range
    if(lastDirection_back != currentDirection_back) {
      lastDirection_back = currentDirection_back;
      currentPatternIndex_back = 0;
      //isKnitting = true;
      if(currentDirection_back == DIRECTION_RIGHT_LEFT) {
        sendCommand(COM_CMD_DIRECTION, "RTL_back");
      } 
      else {
        sendCommand(COM_CMD_DIRECTION, "LTR_back");
      }
    }
  }

  //AutoCalibrate
  passapCalibrateArray_back[passapCalibratePointer_back & 0x07] = currentPinChangeValue_back;
  passapCalibrateArray_back[(passapCalibratePointer_back+1) & 0x07] = currentPinChangeOppositeValue_back;

  if (passapCalibratePointer_back > 8){ // 16
    // read last 16 digits of passapCalibrateArray
    int found = 1;
    for (int i=0; i<8; i++){
      if (passapCalibrateArray_back[(passapCalibratePointer_back-i+2) & 0x07] !=
        passaptestpattern_back[i]) {
        found = 0;
        break;
      }
    }
    if (found){
      //sendCommand(COM_CMD_RESPONSE, "Calibrate_back");
      //calibrate
      currentCursorPosition_back = 1;
      passapCalibratePointer_back = 0;
    }
  }
  passapCalibratePointer_back = passapCalibratePointer_back +2;
}



