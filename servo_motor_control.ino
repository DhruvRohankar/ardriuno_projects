#include <Servo.h>

// Define servo objects
Servo servo1;


void setup() {
  // Attach servo to the control pin
  servo1.attach(8);
}

void loop() {
servo1.write(90);   // Rotate the servo to 90 degrees
delay(1000);       // Wait for 1 second
servo1.write(0);  // Rotate the servo to 0 degrees
delay(1000);       // Wait for 1 second
}


