#define TRIG_PIN 9
#define ECHO_PIN 10

void setup() {
    Serial.begin(9600); // Start serial communication
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
}

void loop() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH);
    float distance = duration * 0.0343 / 2; // Convert to cm

    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");

    delay(500); // Wait before next reading
}


