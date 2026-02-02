#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

/*
 * ARDUINO NANO MP3 PLAYER (MP3-TF-16P / DFPlayer Mini)
 * * WIRING:
 * - DFPlayer VCC -> 5V
 * - DFPlayer GND -> GND
 * - DFPlayer RX  -> Nano D11 (Use 1k Ohm resistor in series)
 * - DFPlayer TX  -> Nano D10
 * - Button 1     -> Nano D2 (and GND)
 * - Button 2     -> Nano D3 (and GND)
 * - Button 3     -> Nano D4 (and GND)
 * * SD CARD SETUP:
 * Create a folder named "mp3" and name files "0001.mp3", "0002.mp3", "0003.mp3"
 */

// Pins for Software Serial
SoftwareSerial mySoftwareSerial(10, 11); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

// Button Pins
const int button1 = 2;
const int button2 = 3;
const int button3 = 4;

void setup() {
  mySoftwareSerial.begin(9600);
  Serial.begin(115200);
  
  // Set up button pins with internal pull-up resistors
  pinMode(button1, INPUT_PULLUP);
  pinMode(button2, INPUT_PULLUP);
  pinMode(button3, INPUT_PULLUP);

  Serial.println(F("Initializing DFPlayer..."));

  if (!myDFPlayer.begin(mySoftwareSerial)) {
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true);
  }
  
  Serial.println(F("DFPlayer Mini online."));
  
  // Set volume (0 to 30)
  myDFPlayer.volume(20); 
}

void loop() {
  // Check Button 1
  if (digitalRead(button1) == LOW) {
    Serial.println(F("Button 1 pressed: Playing File 0001.mp3"));
    myDFPlayer.playMp3Folder(1); 
    delay(500); // Debounce
  }

  // Check Button 2
  if (digitalRead(button2) == LOW) {
    Serial.println(F("Button 2 pressed: Playing File 0002.mp3"));
    myDFPlayer.playMp3Folder(2);
    delay(500); // Debounce
  }

  // Check Button 3
  if (digitalRead(button3) == LOW) {
    Serial.println(F("Button 3 pressed: Playing File 0003.mp3"));
    myDFPlayer.playMp3Folder(3);
    delay(500); // Debounce
  }
}