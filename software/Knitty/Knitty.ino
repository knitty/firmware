
#define INT_0       0
#define INT_1       1

#define PIN_IFDR      3          // Green
#define PIN_CSENSE    2          // Yellow
#define PIN_CREF      4          // White
#define PIN_SEL       9          // Blue,  Pattern

#define PIN_BUTTON    12

#define INT_IFDR      INT_1
#define INT_ENCODER   INT_0


#define DIRECTION_UNKNOWN       0
#define DIRECTION_LEFT_RIGHT   -1
#define DIRECTION_RIGHT_LEFT    1


#define PIN_NEEDLE       PIN_SEL
#define NEEDLE_ACTIVE    HIGH
#define NEEDLE_INACTIVE  LOW

const unsigned char PATTERN_LENGTH = 30;

char currentDirection = DIRECTION_UNKNOWN;
char lastDirection = DIRECTION_UNKNOWN;

signed int endPatternIndex = 10000;
signed int currentCursorPosition = 0;
unsigned int currentPatternIndex = 0;

char knitPattern[PATTERN_LENGTH] = {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0};


void setup() {
  Serial.begin(115200);
  Serial.println("Hallo Knitty, hello World!");

  // Schwurbel
  endPatternIndex = 10000;
  
  // Setup CURSOR pin change interrupt
  //pinMode(PIN_IFDR, INPUT);
  //attachInterrupt(INT_IFDR, interruptPinChangeCarriageConnect, RISING);
  
  // Setup PHOTO SENSOR pin change interrupt
  pinMode(PIN_CSENSE, INPUT);
  pinMode(PIN_CREF, INPUT);
  attachInterrupt(INT_ENCODER, interruptPinChangeEncoder, CHANGE);
  
  // Setup Needle
  pinMode(PIN_NEEDLE, OUTPUT);  
  digitalWrite(PIN_NEEDLE, LOW);
  
  // Setup HID
  pinMode(PIN_BUTTON, INPUT);
}

void loop() {
  char currentButtonStatus = digitalRead(PIN_BUTTON);
  if(currentButtonStatus == 1) {    
    Serial.println("Reset cursor counter to zero");
    currentCursorPosition = 0;
    endPatternIndex = 10000;
    currentPatternIndex = 0;
  }
}

void setNeedleByCursor(char cursorPosition) {
 
  if(cursorPosition >= PATTERN_LENGTH && cursorPosition < 0) {
    Serial.print("Warning! Needle out of range");
    Serial.println(cursorPosition, 10);
    return;
  }
 
  setNeedle(knitPattern[cursorPosition]);
} 

void setNeedle(char state) {  
  if(state == 1) {
    digitalWrite(PIN_NEEDLE, HIGH);
  } else {
    digitalWrite(PIN_NEEDLE, LOW);  
  }
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
  Serial.println(currentCursorPosition, 10);
  
  // Check if we're in range of our needles
  if((currentDirection == DIRECTION_RIGHT_LEFT && currentCursorPosition > 0) ||
     (currentDirection == DIRECTION_LEFT_RIGHT && currentCursorPosition <= endPatternIndex)) {
        
    Serial.print("PI: ");
    Serial.println(currentPatternIndex, 10);

    if(currentPatternIndex >= PATTERN_LENGTH) {
         
       setNeedle(0);
       currentPatternIndex = 0;
       
       if(currentDirection == DIRECTION_RIGHT_LEFT) {
         Serial.println("RTL Pattern finished!");
         endPatternIndex = currentCursorPosition;
       } else {
         Serial.println("LTR Pattern finished!");
       }
    } else {
          
      if(currentDirection == DIRECTION_RIGHT_LEFT && currentPinChangeValue == 0) {
        setNeedleByCursor(currentPatternIndex);
        currentPatternIndex++;
      }
      
      if(currentDirection == DIRECTION_LEFT_RIGHT && currentPinChangeValue == 1) {
        setNeedleByCursor(currentPatternIndex);
        currentPatternIndex++;
      }
    }
  }
  
  if(lastDirection != currentDirection) {
    lastDirection = currentDirection;
    currentPatternIndex = 0;
  }
}

void interruptPinChangeCarriageConnect() {  
  Serial.println("IFDR up");
}
