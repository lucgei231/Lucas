#include <Arduino.h>
#include <ESP32Servo.h>

// --- Global variables ---
Servo myServo;
Servo myServo2;
Servo myServo3;

float OFFSET1 = 115;                      //motor1 offset along x_axis
float OFFSET2 = 180;                      //motor2 offset along x_axis
float YAXIS = 465;                        //motor heights above (0,0)
float LENGTH = 255;                       //length of each arm-segment
float SCALE_FACTOR = 1;                   //drawing scale (1 = 100%)
float ARC_MAX = 2;                        //maximum arc-length (controls smoothness)

int THIS_X = 0;                         //this X co-ordinate (rounded)
int THIS_Y = 0;                         //this Y co-ordinate (rounded)
int LAST_X = 0;                         //last X co-ordinate (rounded)
int LAST_Y = 0;                         //last Y co-ordinate (rounded)



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

/*
  This routine calculates the the number of motor steps to reach (x,y) from 12 o'clock .
  Global variables STEPS1, STEPS2 hold the required number of steps
*/
void calculate_steps(float x, float y) {

  float distance1;            //pen distance to motor1
  float distance2;            //pen distance to motor2
  float angle1;               //motor1 angle
  float angle2;               //motor2 angle

  distance1 = sqrt((OFFSET1 - x) * (OFFSET1 - x) + (YAXIS - y) * (YAXIS - y));
  distance2 = sqrt((OFFSET2 - x) * (OFFSET2 - x) + (YAXIS - y) * (YAXIS - y));

  // ----- calculate motor1 angle when pen at (x,y)
  if (x > OFFSET1) {
    angle1 = PI + acos(distance1 / (2 * LENGTH)) - atan((x - OFFSET1) / (YAXIS - y)); //radians
  } else {
    angle1 = PI + acos(distance1 / (2 * LENGTH)) + atan((OFFSET1 - x) / (YAXIS - y)); //radians
  }

  // ----- calculate motor2 angle when pen at start position (0,0)
  if (x > OFFSET2) {
    angle2 = PI - acos(distance2 / (2 * LENGTH)) - atan((x - OFFSET2) / (YAXIS - y)); //radians
  } else {
    angle2 = PI - acos(distance2 / (2 * LENGTH)) + atan((OFFSET2 - x) / (YAXIS - y)); //radians
  }

  // ----- calculate steps required to reach (x,y) from 12 o'clock
  STEPS1 = round(angle1 * RAD_TO_DEG * STEPS_PER_DEG);
  STEPS2 = round(angle2 * RAD_TO_DEG * STEPS_PER_DEG);
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