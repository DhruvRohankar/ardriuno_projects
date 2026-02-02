#include <Servo.h>

Servo myServo;

const int trigPin = 2;
const int echoPin = 3;

long duration;
int distance;

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  myServo.attach(9);     // Servo signal pin
  myServo.write(0);      // Initial position
}

void loop() {
  // Ultrasonic trigger
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read echo
  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;

  // If object detected within 10 cm
  if (distance > 0 && distance <= 10) {
    myServo.write(90);   // Move to 85degree
    delay(3000);         // Hold for 3 seconds
    myServo.write(0);    // Return to 0°
    delay(1000);         // Small delay to avoid repeat
  }
}