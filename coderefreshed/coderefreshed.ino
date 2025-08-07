#include <ESP32Servo.h>
int p1;
int p2;
Servo servo1;
Servo servo2;
Servo servo3;
void setup() {
Serial.begin(9600);
pinMode(12, INPUT_PULLUP);
pinMode(13, INPUT_PULLUP);
pinMode(32, INPUT);
pinMode(33, INPUT);
servo1.attach(16, 500, 2500);
servo2.attach(17, 500, 2500);
servo3.attach(18, 500, 2500);
}
void loop() {
p1 = analogRead(32);
p2 = analogRead(33);
// Invert mapping: higher pot value = lower angle
int angle1 = map(p1, 0, 4095, 180, 0);
int angle2 = map(p2, 0, 4095, 180, 0);
servo2.write(angle1);
servo3.write(angle2);
Serial.print("P1: ");
Serial.print(angle1);
Serial.print(" , P2: ");
Serial.println(angle2);
}