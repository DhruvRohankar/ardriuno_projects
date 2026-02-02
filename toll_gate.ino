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
    // Slow motion servo movement
    int targetAngle = 90;
    int currentAngle = myServo.read();
    int increment = (targetAngle - currentAngle) / 15; // Adjust '10' for speed (smaller = faster)

    while (currentAngle != targetAngle) {
      currentAngle += increment;
      if (currentAngle > targetAngle) {
        currentAngle = targetAngle;
      }
      if (currentAngle < 0) {
        currentAngle = 0;
      }
      myServo.write(currentAngle);
      delay(15); // Adjust this delay for speed (smaller = faster)
    }

    delay(4000);         // Hold for 3 seconds

    // Slow motion servo movement back to 0
    targetAngle = 0;
    currentAngle = myServo.read();
    increment = (targetAngle - currentAngle) / 10;

    while (currentAngle != targetAngle) {
      currentAngle += increment;
      if (currentAngle > targetAngle) {
        currentAngle = targetAngle;
      }
      if (currentAngle < 0) {
        currentAngle = 0;
      }
      myServo.write(currentAngle);
      delay(15); // Adjust this delay for speed (smaller = faster)
    }

    delay(1000);         // Small delay to avoid repeat
  }
}
