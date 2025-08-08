#include <Arduino.h>
#include <ESP32Servo.h>

#define PI 3.1415926535897932384626433832795
#define HALF_PI 1.5707963267948966192313216916398
#define TWO_PI 6.283185307179586476925286766559
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105

Servo myServo;
Servo myServo2;
Servo myServo3;

float OFFSET1 = 24;                      //motor1 offset along x_axis
float OFFSET2 = 62;                      //motor2 offset along x_axis
float YAXIS = 75;                        //motor heights above (0,0)
float LENGTH_1 = 41;                       //length of short arm-segment
float LENGTH_2 = 50;                       //length of long arm-segment


float SCALE_FACTOR = 1;                   //drawing scale (1 = 100%)
float ARC_MAX = 2;                        //maximum arc-length (controls smoothness)

int THIS_X = 0;                         //this X co-ordinate (rounded)
int THIS_Y = 0;                         //this Y co-ordinate (rounded)
int LAST_X = 0;                         //last X co-ordinate (rounded)
int LAST_Y = 0;                         //last Y co-ordinate (rounded)
int last_pot_1 = 0;
int last_pot_2 = 0;


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

void calculate_angles(float x, float y) {

  float distance1;            //pen distance to motor1
  float distance2;            //pen distance to motor2
  float angle1;               //motor1 angle
  float angle2;               //motor2 angle

  distance1 = sqrt((OFFSET1 - x) * (OFFSET1 - x) + (YAXIS - y) * (YAXIS - y));
  distance2 = sqrt((OFFSET2 - x) * (OFFSET2 - x) + (YAXIS - y) * (YAXIS - y));
  float arg1 = distance1 / (LENGTH_1 + LENGTH_2);
  float arg2 = distance2 / (LENGTH_1 + LENGTH_2);
  arg1 = constrain(arg1, -1.0, 1.0);
  arg2 = constrain(arg2, -1.0, 1.0);

  // ----- calculate motor1 angle when pen at (x,y)
  if (x > OFFSET1) {
    angle1 = PI + acos(arg1) - atan((x - OFFSET1) / (YAXIS - y)); //radians
  } else {
    angle1 = PI + acos(arg1) + atan((OFFSET1 - x) / (YAXIS - y)); //radians
  }

  // ----- calculate motor2 angle when pen at start position (0,0)
  if (x > OFFSET2) {
    angle2 = PI - acos(arg2) - atan((x - OFFSET2) / (YAXIS - y)); //radians
  } else {
    angle2 = PI - acos(arg2) + atan((OFFSET2 - x) / (YAXIS - y)); //radians
  }


    //   // ----- calculate steps required to reach (x,y) from 12 o'clock
    //   STEPS1 = round(angle1 * RAD_TO_DEG * STEPS_PER_DEG);
    //   STEPS2 = round(angle2 * RAD_TO_DEG * STEPS_PER_DEG);
    angle1 = angle1 * RAD_TO_DEG;
    angle2 = angle2 * RAD_TO_DEG;
    Serial.print("Move to: ");
    Serial.print(angle1);
    Serial.print(", ");
    Serial.println(angle2);

    angle1 = (270 - angle1);
    angle2 = (270 - angle2);
    Serial.print(angle1);
    Serial.print(" , ");
    Serial.println(angle2);

    myServo2.write(angle2);
    myServo3.write(angle1);
}

void loop (){
    // --- Potentiometer/slider control logic ---
    int pot32 = analogRead(32);
    int pot33 = analogRead(33);

    // int angle32 = map(pot32, 0, 4095, 180, 0);
    // If the difference between pot32 and last_pot_1 is more than 5, update the servo position
    // if (abs(pot32 - last_pot_1) > 20) {
    //     myServo2.write(angle32);
    // }


    // int angle33 = map(pot33, 0, 4095, 180, 0);
    // If the difference between pot33 and last_pot_2 is more than 5, update the servo position
    // if (abs(pot33 - last_pot_2) > 20) {
    //     myServo3.write(angle33);
    // }

    // Serial.print("Potentiometer Values: ");
    // Serial.print(pot32);
    // Serial.print(" , ");
    // Serial.println(pot33);

    // Serial.print("Angle Values: ");
    // Serial.print(angle32);
    // Serial.print(" , ");
    // Serial.println(angle33);

    delay(1000);
    for (int i = 0; i < 40; i++) {
      calculate_angles(10, 10 + i);
      delay(100);
    }
    for (int i = 0; i < 40; i++) {
      calculate_angles(10 + i, 50);
      delay(100);
    }

    last_pot_1 = pot32;
    last_pot_2 = pot33;
}