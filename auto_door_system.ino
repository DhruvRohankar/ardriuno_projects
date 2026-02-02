#include <Servo.h>

#define TRIG_PIN 11   // Ultrasonic sensor trigger pin
#define ECHO_PIN 10   // Ultrasonic sensor echo pin
#define SERVO_PIN 3   // Servo motor control pin

Servo doorServo; // Create a servo object
int currentAngle = 0; // Track current servo position

void setup() {
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    doorServo.attach(SERVO_PIN);
    doorServo.write(currentAngle); // Start with door closed
    Serial.begin(9600);
}

long getDistance() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    long duration = pulseIn(ECHO_PIN, HIGH);
    long distance = duration * 0.034 / 2; // Convert time to distance (cm)
    return distance;
}

void smoothServoMove(int targetAngle) {
    if (targetAngle > currentAngle) {  // If target is greater, move up
        for (int angle = currentAngle; angle <= targetAngle; angle++) {
            doorServo.write(angle);
            delay(10); // Smooth movement delay
        }
    } else { // If target is lower, move down
        for (int angle = currentAngle; angle >= targetAngle; angle--) {
            doorServo.write(angle);
            delay(10); // Smooth movement delay
        }
    }
    currentAngle = targetAngle; // Update current position
}

void loop() {
    long distance = getDistance();
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
    
    if (distance > 0 && distance <= 30) { // If object detected within 30 cm
        smoothServoMove(90); // Slowly open the door
        delay(3000); // Keep it open for 3 seconds
    } else {
        smoothServoMove(0); // Slowly close the door
    }
    delay(500);
}
