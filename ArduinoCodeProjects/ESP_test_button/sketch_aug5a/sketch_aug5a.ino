#include <Stepper.h>

const int stepsPerRevolution = 2048;  // Aantal stappen per volledige rotatie van de motor
const int motorSpeed = 50;  // Snelheid in RPM
const int movePin = 10;  // Pin voor knop 1

// ULN2003 Motor Driver Pins
#define IN1 1
#define IN2 2
#define IN3 3
#define IN4 13

Stepper myStepper(stepsPerRevolution, IN1, IN3, IN2, IN4);

void setup() {
  Serial.begin(115200);

  // Stel de pinnen in
  pinMode(movePin, INPUT_PULLUP);  // Knop 1

  // Stel de snelheid in
  myStepper.setSpeed(motorSpeed);
}

void loop() {
  // Lees de staat van de knop
  int state = digitalRead(movePin);

  if (state == LOW) {
    // Knop 1 is ingedrukt, beweeg de motor één stap
    myStepper.step(1);
    //Serial.println("Knop 1 is ingedrukt en motor beweegt");
  } else {
    // Knop 1 is niet ingedrukt, geen actie
    //Serial.println("Knop 1 is niet ingedrukt");
  }

  // Optionele vertraging om de seriële output leesbaar te maken en motor niet te snel te laten draaien
  delay(50);
}
