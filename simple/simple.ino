#include <Arduino.h>
#include <ESP32Servo.h>

// --- Global variables ---
Servo myServo;
Servo myServo2;
Servo myServo3;

// --- Setup and loop ---
void setup (){
    Serial.begin(9600);

    Serial.println("Booting...");

    pinMode(12, INPUT_PULLUP);
    pinMode(13, INPUT_PULLUP);
    pinMode(32, INPUT);
    pinMode(33, INPUT);
    pinMode(2, OUTPUT); // Add this to control D2 LED
    myServo.setPeriodHertz(50);
    myServo.attach(16, 500, 2500);
    myServo2.setPeriodHertz(50);
    myServo2.attach(17, 500, 2500);
    myServo3.setPeriodHertz(50);
    myServo3.attach(18, 500, 2500);
}

void loop (){
    // --- Potentiometer/slider control logic ---
    int pot32 = analogRead(32);
    int pot33 = analogRead(33);
    int angle32 = map(pot32, 0, 4095, 180, 0);
    int angle33 = map(pot33, 0, 4095, 180, 0);

    myServo2.write(angle32);
    myServo3.write(angle33);

    Serial.print("Potentiometer Values: ");
    Serial.print(pot32);
    Serial.print(" , ");
    Serial.println(pot33);

    Serial.print("Angle Values: ");
    Serial.print(angle32);
    Serial.print(" , ");
    Serial.println(angle33);

}