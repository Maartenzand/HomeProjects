#include <AccelStepper.h>

#define STEP_PIN        3
#define DIR_PIN         2
#define ENABLE_PIN      1

#define MAX_SPEED         1000
#define CURRENT_SPEED     750
#define MAX_ACCELERATION  500 

AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

void setup() {
  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, LOW); 
  
  stepper.setMaxSpeed(MAX_SPEED);  
  stepper.setAcceleration(MAX_ACCELERATION);  
  
  stepper.setSpeed(CURRENT_SPEED);  
}

void loop() {
  stepper.runSpeed();
}