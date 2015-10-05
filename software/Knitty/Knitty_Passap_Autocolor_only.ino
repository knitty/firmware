//////////////////////////////////////////////////////////////////////////////
// Knitty Project
//
// Author: ptflea
//

//Servo
#include <Servo.h>

Servo servoColour12;  // create servo object to control a servo
Servo servoColour34;


//////////////////////////////////////////////////////////////////////////////
// General purpose definitions


#define PIN_BUTTON_1  7         // Button_1 (activate colour change)
#define BUTTONDELAY   20        // delay for Button_1
// PIN 8 and 9 are for the color change servos


long buttonLastChecked = 0; // variable to limit the button getting checked every cycle
int button_1_State = 0;     // status of button_1
int button_1_Hold = 0; // toggle state of Button_1




//////////////////////////////////////////////////////////////////////////////
// Knitty Serial Protocol

// Receive commands

#define COM_CMD_RESPONSE     'R'
#define COM_CMD_DEBUG        'V'
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
  sendCommand(COM_CMD_RESPONSE, "up and running");
  // Button Input
  pinMode(PIN_BUTTON_1, INPUT);


}

void executeCommand(unsigned char cmd, String payload) {

  switch(cmd) {

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


        parserReceivedPayload += buffer;
 
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
        sendCommand(COM_CMD_RESPONSE, "ColourChanger ON");
        button_1_Hold = LOW;
      }
    } else if (button_1_Hold == LOW) {
        // Send
        sendCommand(COM_CMD_RESPONSE, "ColourChanger OFF");
        button_1_Hold = HIGH;
    }
    buttonLastChecked = millis(); // reset the lastChecked value
  }

}


