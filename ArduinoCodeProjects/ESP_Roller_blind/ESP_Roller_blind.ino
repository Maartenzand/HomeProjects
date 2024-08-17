#include <Stepper.h>

const int stepsPerRevolution = 2048;  // has64:1 gear ratio
const int motorSpeed = 15;  // RPM 
const int movePin1 = 10;  // button1
const int movePin2 = 11; // button2

// ULN2003 Motor Driver Pins
#define IN1 1
#define IN2 2
#define IN3 3
#define IN4 13

bool moved1 = false; 
bool moved2 = false; 

Stepper myStepper(stepsPerRevolution, IN1, IN3, IN2, IN4);

void setup() {
  Serial.begin(115200);

  // Pinmode for buttons
  pinMode(movePin1, INPUT_PULLUP);
  pinMode(movePin2, INPUT_PULLUP);

  myStepper.setSpeed(motorSpeed);
}

void loop() {
  int state = digitalRead(movePin1);
  int state2 = digitalRead(movePin2);

  if (state == LOW) {
    myStepper.step(10); 
    Serial.println("Button 1 hit and motor moves");
    moved1 = true; 
  } else if (state2 == LOW){
    myStepper.step(-10); 
    Serial.println("Button 2 hit and motor moves");
    moved2 = true;
  } else {
    //Serial.println("Button 1 or 2 not pressed");

    // Check if we have moved
    if (moved1 == true || moved2 == true)
    {
      // make sure motor breaks instead of freefall    
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, HIGH);

      if (digitalRead(IN1) == HIGH && digitalRead(IN2) == HIGH)
      {
        Serial.println("Break motors");
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, LOW);
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, LOW);
        moved1 = false;
        moved2 = false;
      }
    }
  }

  delay(10);
}