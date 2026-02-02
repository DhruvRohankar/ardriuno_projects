/*
 * ARDUINO NANO BLUETOOTH RC CAR – HC-05
 * Motor Driver: BTS7960 / IBT-2
 */

#include <SoftwareSerial.h>

// ================= BLUETOOTH =================
SoftwareSerial BT(2, 3); // RX, TX

char command;

// ================= LEFT MOTOR (BTS7960 #1) =================
const int L_EN   = 8;
const int L_RPWM = 5;
const int L_LPWM = 6;

// ================= RIGHT MOTOR (BTS7960 #2) =================
const int R_EN   = 11;
const int R_RPWM = 9;
const int R_LPWM = 10;

// ================= SETTINGS =================
int motorSpeed = 100;   // 0–255 PWM speed

void setup() {
  // Motor Pins
  pinMode(L_EN, OUTPUT);
  pinMode(L_RPWM, OUTPUT);
  pinMode(L_LPWM, OUTPUT);

  pinMode(R_EN, OUTPUT);
  pinMode(R_RPWM, OUTPUT);
  pinMode(R_LPWM, OUTPUT);

  // Enable Motor Drivers
  digitalWrite(L_EN, HIGH);
  digitalWrite(R_EN, HIGH);

  // Bluetooth
  BT.begin(9600);

  stopCar();
}

void loop() {
  if (BT.available()) {
    command = BT.read();

    switch (command) {
      case 'F':
        moveForward();
        break;

      case 'B':
        moveBackward();
        break;

      case 'L':
        turnLeft();
        break;

      case 'R':
        turnRight();
        break;

      case 'S':
        stopCar();
        break;
    }
  }
}

// ================= MOTOR FUNCTIONS =================

void moveForward() {
  analogWrite(L_RPWM, motorSpeed);
  analogWrite(L_LPWM, 0);
  analogWrite(R_RPWM, motorSpeed);
  analogWrite(R_LPWM, 0);
}

void moveBackward() {
  analogWrite(L_RPWM, 0);
  analogWrite(L_LPWM, motorSpeed);
  analogWrite(R_RPWM, 0);
  analogWrite(R_LPWM, motorSpeed);
}

void turnLeft() {
  analogWrite(L_RPWM, 0);
  analogWrite(L_LPWM, motorSpeed);
  analogWrite(R_RPWM, motorSpeed);
  analogWrite(R_LPWM, 0);
}

void turnRight() {
  analogWrite(L_RPWM, motorSpeed);
  analogWrite(L_LPWM, 0);
  analogWrite(R_RPWM, 0);
  analogWrite(R_LPWM, motorSpeed);
}

void stopCar() {
  analogWrite(L_RPWM, 0);
  analogWrite(L_LPWM, 0);
  analogWrite(R_RPWM, 0);
  analogWrite(R_LPWM, 0);
}
