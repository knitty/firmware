
#define INT_0       0
#define INT_1       1

#define PIN_IFDR      3          // Green
#define PIN_CSENSE    2          // Yellow
#define PIN_CREF      4          // White
#define PIN_SEL       9          // Blue,  Pattern

#define INT_IFDR      INT_1
#define INT_ENCODER   INT_0


#define DIRECTION_UNKNOWN      0
#define DIRECTION_LEFT_RIGHT   1
#define DIRECTION_RIGHT_LEFT   2


#define PIN_NEEDLE       PIN_SEL
#define NEEDLE_ACTIVE    HIGH
#define NEEDLE_INACTIVE  LOW

#define PATTERN_LENGTH   30

boolean isPatternActive = false;

char currentDirection = DIRECTION_UNKNOWN;
char lastDirection = DIRECTION_UNKNOWN;

char activePinCount = 0;
char currentLine = 0;

char knitPattern[PATTERN_LENGTH] = {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0};


void setup() {
  Serial.begin(115200);
  Serial.println("Hallo Knitty, hello World!");

  // Setup CURSOR pin change interrupt
  pinMode(PIN_IFDR, INPUT);
  attachInterrupt(INT_IFDR, interruptPinChangeIfdr, RISING);
  
  // Setup PHOTO SENSOR pin change interrupt
  pinMode(PIN_CSENSE, INPUT);
  pinMode(PIN_CREF, INPUT);
  attachInterrupt(INT_ENCODER, interruptPinChangeEncoder, CHANGE);
  
  // Setup Needle
  pinMode(PIN_NEEDLE, OUTPUT);  
  digitalWrite(PIN_NEEDLE, LOW);
}

void loop() {
  
}

void setNeedleByCursor(char cursorPosition) {
 
  if(cursorPosition >= PATTERN_LENGTH) {
    //Serial.print("Warning! Needle out of range");
    //Serial.println(cursorPosition, 10);
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
  
  
  
  if(isPatternActive && currentPinChangeValue == 0) { 
    
    // We've reached the end of our pattern, so we deactive our output
    // and end pattern writing
    if(activePinCount >= PATTERN_LENGTH) {
      
      setNeedle(NEEDLE_INACTIVE);  
      isPatternActive = false;
      currentLine++;
      
      Serial.println("Pattern finished!");
      Serial.println(currentLine, 10);

    } else {
      setNeedleByCursor(activePinCount);   
    }
      
    activePinCount += 1;
    
  }
  
  if(lastDirection != currentDirection) {
    lastDirection = currentDirection;
    
    // Direction changed to the pin change interrupt
    activePinCount = 0;
    isPatternActive = false;
    
    if(currentDirection == DIRECTION_RIGHT_LEFT) {
    //  Serial.println("R->L");  
    } else {
    //  Serial.println("L->R");
    }
  }
  //Serial.print("CREF, cnt: ");
  //Serial.println(cursorCount, 10);
}

void interruptPinChangeIfdr() {
  Serial.println("Pin RISING on IFDR");  
  isPatternActive = true;
}
