#include <Arduino.h>
#line 1 "D:\\Desktop\\DrawBot\\Lucas\\mytest\\mytest.ino"
// -------------------------------
// GLOBALS
// -------------------------------

// ----- constants
#define PI 3.1415926535897932384626433832795
#define HALF_PI 1.5707963267948966192313216916398
#define TWO_PI 6.283185307179586476925286766559
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105

// ----- motor definitions
#define STEPS_PER_DEG 1600/360      //angular rotation for microstepped NEMA17
#define NUDGE 1*STEPS_PER_DEG       //rotate the motor 1 degrees (change number to suit)
#define CW 1                        //motor directions
#define CCW 0
#define DIR1 8                      //EasyDriver ports
#define DIR2 10
#define STEP1 9
#define STEP2 11

long
  PULSE_WIDTH = 5,                  //easydriver step pulse-width (uS)
  DELAY_MIN = 20000,                //minimum inter-step delay (uS) between motor steps (controls speed)
  DELAY1,                           //inter-step delay for motor1 (uS)
  DELAY2,                           //inter-step delay for motor2 (uS)
  STEPS1,                           //motor1 steps from 12 o'clock to reach this location
  STEPS2;                           //motor2 steps from 12 o'clock to reach this location

// ----- plotter definitions
#define BAUD 9600
#define XOFF 0x13                   //pause transmission (19 decimal)
#define XON 0x11                    //resume transmission (17 decimal)
#define PEN 3
#define OFFSET1 115                 //motor1 offset along x_axis
#define OFFSET2 180                 //motor2 offset along x_axis
#define YAXIS 465                   //motor heights above (0,0)
#define LENGTH 255                  //length of each arm-segment
  
float 
  SCALE_FACTOR = 1;                 //drawing scale (1 = 100%)
 
// ----- gcode definitions
#define STRING_SIZE 128             //string size

char  
  BUFFER[STRING_SIZE + 1],
  INPUT_CHAR;
  
String 
  INPUT_STRING,
  SUB_STRING;
  
int   
  INDEX = 0,                        //buffer index
  START,                            //used for sub_string extraction
  FINISH;
  
float
  X,                                //gcode float values held here
  Y,
  I,
  J;

// -----------------------
// SETUP
// -----------------------
#line 68 "D:\\Desktop\\DrawBot\\Lucas\\mytest\\mytest.ino"
void setup();
#line 141 "D:\\Desktop\\DrawBot\\Lucas\\mytest\\mytest.ino"
void loop();
#line 168 "D:\\Desktop\\DrawBot\\Lucas\\mytest\\mytest.ino"
void menu();
#line 189 "D:\\Desktop\\DrawBot\\Lucas\\mytest\\mytest.ino"
void process();
#line 448 "D:\\Desktop\\DrawBot\\Lucas\\mytest\\mytest.ino"
void calculate_steps(float x, float y);
#line 483 "D:\\Desktop\\DrawBot\\Lucas\\mytest\\mytest.ino"
void move_to(float X, float Y );
#line 555 "D:\\Desktop\\DrawBot\\Lucas\\mytest\\mytest.ino"
void calculate_delays(long steps1, long steps2);
#line 587 "D:\\Desktop\\DrawBot\\Lucas\\mytest\\mytest.ino"
void step_motor1(bool dir1);
#line 598 "D:\\Desktop\\DrawBot\\Lucas\\mytest\\mytest.ino"
void step_motor2(bool dir2);
#line 609 "D:\\Desktop\\DrawBot\\Lucas\\mytest\\mytest.ino"
void step1_cw();
#line 625 "D:\\Desktop\\DrawBot\\Lucas\\mytest\\mytest.ino"
void step1_ccw();
#line 641 "D:\\Desktop\\DrawBot\\Lucas\\mytest\\mytest.ino"
void step2_cw();
#line 657 "D:\\Desktop\\DrawBot\\Lucas\\mytest\\mytest.ino"
void step2_ccw();
#line 675 "D:\\Desktop\\DrawBot\\Lucas\\mytest\\mytest.ino"
void pen_up();
#line 685 "D:\\Desktop\\DrawBot\\Lucas\\mytest\\mytest.ino"
void pen_down();
#line 693 "D:\\Desktop\\DrawBot\\Lucas\\mytest\\mytest.ino"
void abc();
#line 824 "D:\\Desktop\\DrawBot\\Lucas\\mytest\\mytest.ino"
void target();
#line 889 "D:\\Desktop\\DrawBot\\Lucas\\mytest\\mytest.ino"
void radials();
#line 68 "D:\\Desktop\\DrawBot\\Lucas\\mytest\\mytest.ino"
void setup()
{
  // ----- initialise motor1
  pinMode(DIR1,OUTPUT);
  pinMode(STEP1,OUTPUT);
  digitalWrite(DIR1,CW);
  delayMicroseconds(PULSE_WIDTH);
  digitalWrite(STEP1,HIGH);
  
  // ----- initialise motor2
  pinMode(DIR2,OUTPUT);
  pinMode(STEP2,OUTPUT);
  digitalWrite(DIR2,CW);
  delayMicroseconds(PULSE_WIDTH);
  digitalWrite(STEP2,HIGH);

  // ----- calculate motor steps to reach (0,0)
  calculate_steps(0,0);

  // ----- pen-lift
  pinMode(PEN, OUTPUT);                                       //D3
  TCCR2A = _BV(COM2B1) | _BV(COM2B0) | _BV(WGM20);            //PWM
  TCCR2B = _BV(WGM22) | _BV(CS22) | _BV(CS21) | _BV(CS20);    //div 1024
  OCR2A = 156;                                                //20mS period
  OCR2B = 148;                                                //2mS period (pen-up)

  /*
    The above pen-lift comprises a standard servo which requires a 1 mS pulse
    for pen down, or a 2mS pulse for pen up, with a fixed period of 20mS.

    The Arduino "bit value" macro, #define _BV(x) (1 << x), is used to
    set the Timer2 mode to "phase-correct PWM" with a variable "top-limit".
    In this mode the timer counts up to the value entered into register OCR2A
    then back down to zero.

    The following values were used to obtain a 20mS period at pin D3:
      clock:        16 MHz
    prescaler:      1024
    top-limit (OCR2A):  156
    period:       16MHz/1024/(156*2) = 50Hz (20mS)

    If you enter a value into register OCR2B that is LESS than the value in
    register OCR2A then timer2 will will pass through the value in OCR2B
    twice ... once on the way up ... and once on the way down. The duration
    of the output pulse on pin D3 is the time that the count in OCR2A is
    greater than the value in OCR2B.

    A value of 148 entered into OCR2B creates a 1mS pulse:
    period:       156-148)*20mS/156 = 1mS (pen-up)

    A value of 140 entered into OCR2B creates a 2mS pulse):
    period:     156-140)*20mS/156 = 2mS (pen-down)
  */

  // ----- plotter setup
  memset(BUFFER, '\0', sizeof(BUFFER));     //fill with string terminators
  INPUT_STRING.reserve(STRING_SIZE);
  INPUT_STRING = "";

  // ----- establish serial link
  Serial.begin(BAUD);

  // ----- flush the buffers
  Serial.flush();                           //clear TX buffer
  while (Serial.available()) Serial.read(); //clear RX buffer

  // ----- display commands
  menu();
}

//--------------------------------------------------------------------------
// MAIN LOOP
//--------------------------------------------------------------------------
void loop() {

  // -------------------------
  // get the next instruction
  // -------------------------
  while (Serial.available()) {
    INPUT_CHAR = (char)Serial.read();         //read character
    Serial.write(INPUT_CHAR);                 //echo character to the screen
    BUFFER[INDEX++] = INPUT_CHAR;             //add char to buffer
    if (INPUT_CHAR == '\n') {                 //check for line feed
      Serial.flush();                         //clear TX buffer
      Serial.write(XOFF);                     //pause transmission
      INPUT_STRING = BUFFER;                  //convert to string
      process();                              //interpret string and perform task
      memset(BUFFER, '\0', sizeof(BUFFER));   //fill buffer with string terminators
      INDEX = 0;                              //point to buffer start
      INPUT_STRING = "";                      //empty string
      Serial.flush();                         //clear TX buffer
      Serial.write(XON);                      //resume transmission
    }
  }
}

//---------------------------------------------------------------------------
// MENU
// The Arduino F() flash macro is used to conserve RAM.
//---------------------------------------------------------------------------
void menu() {
  Serial.println(F(""));
  Serial.println(F("  ------------------------------------------------------"));
  Serial.println(F("                         MENU"));
  Serial.println(F("  ------------------------------------------------------"));
  Serial.println(F("    MENU ............... menu"));
  Serial.println(F("    G00 X## Y## ........ goto XY (pen-up)"));
  Serial.println(F("    G01 X## Y## ........ goto XY (pen-down)"));
  Serial.println(F("    T1 ................. move pen to 0,0"));
  Serial.println(F("    T2 S##.## .......... set drawing Scale (1=100%)"));
  Serial.println(F("    T3 ................. pen up"));
  Serial.println(F("    T4 ................. pen down"));
  Serial.println(F("    T5 ................. test pattern: ABC"));
  Serial.println(F("    T6 ................. test pattern: target"));
  Serial.println(F("    T7 ................. test pattern: radials"));
  Serial.println(F("  ------------------------------------------------------"));
}

//--------------------------------------------------------------------------
// PROCESS
//--------------------------------------------------------------------------
void process() {

  // ----- convert string to upper case
  INPUT_STRING.toUpperCase();

  // ----------------------------------
  // G00   linear move with pen_up
  // ----------------------------------
  if (INPUT_STRING.startsWith("G00")) {

    // ----- extract X
    START = INPUT_STRING.indexOf('X');
    if (!(START < 0)) {
      FINISH = START + 8;
      SUB_STRING = INPUT_STRING.substring(START + 1, FINISH + 1);
      X = SUB_STRING.toFloat();
    }

    // ----- extract Y
    START = INPUT_STRING.indexOf('Y');
    if (!(START < 0)) {
      FINISH = START + 8;
      SUB_STRING = INPUT_STRING.substring(START + 1, FINISH + 1);
      Y = SUB_STRING.toFloat();
    }

    pen_up();
    move_to(X, Y);
  }

  // ----------------------------------
  // G01   linear move with pen_down
  // ----------------------------------
  if (INPUT_STRING.startsWith("G01")) {

    // ----- extract X
    START = INPUT_STRING.indexOf('X');
    if (!(START < 0)) {
      FINISH = START + 8;
      SUB_STRING = INPUT_STRING.substring(START + 1, FINISH + 1);
      X = SUB_STRING.toFloat();
    }

    // ----- extract Y
    START = INPUT_STRING.indexOf('Y');
    if (!(START < 0)) {
      FINISH = START + 8;
      SUB_STRING = INPUT_STRING.substring(START + 1, FINISH + 1);
      Y = SUB_STRING.toFloat();
    }

    pen_down();
    move_to(X, Y);
  }

  // ----------------------------------
  // G02   clockwise arc with pen_down
  // ----------------------------------
  if (INPUT_STRING.startsWith("G02")) {

    // ----- extract X
    START = INPUT_STRING.indexOf('X');
    if (!(START < 0)) {
      FINISH = START + 8;
      SUB_STRING = INPUT_STRING.substring(START + 1, FINISH + 1);
      X = SUB_STRING.toFloat();
    }

    // ----- extract Y
    START = INPUT_STRING.indexOf('Y');
    if (!(START < 0)) {
      FINISH = START + 8;
      SUB_STRING = INPUT_STRING.substring(START + 1, FINISH + 1);
      Y = SUB_STRING.toFloat();
    }

    pen_down();
    move_to(X, Y);
  }

  // ------------------------------------------
  // G03   counter-clockwise arc with pen_down
  // ------------------------------------------
  if (INPUT_STRING.startsWith("G03")) {

    // ----- extract X
    START = INPUT_STRING.indexOf('X');
    if (!(START < 0)) {
      FINISH = START + 8;
      SUB_STRING = INPUT_STRING.substring(START + 1, FINISH + 1);
      X = SUB_STRING.toFloat();
    }

    // ----- extract Y
    START = INPUT_STRING.indexOf('Y');
    if (!(START < 0)) {
      FINISH = START + 8;
      SUB_STRING = INPUT_STRING.substring(START + 1, FINISH + 1);
      Y = SUB_STRING.toFloat();
    }

    pen_down();
    move_to(X, Y);
  }

  // ----------------------------------
  // MENU
  // ----------------------------------
  if (INPUT_STRING.startsWith("MENU")) {
    menu();
  }

  // ----------------------------------
  // T1   position the pen over 0,0
  // ----------------------------------
  if (INPUT_STRING.startsWith("T1")) {

    // ----- variables
    int step;           //loop counter
    int steps = NUDGE;  //steps motor is to rotate

    // ----- instructions
    Serial.println(F(""));
    Serial.println(F("  ----------------------------------------------"));
    Serial.println(F("    Position the pen over the 0,0 co-ordinate:"));
    Serial.println(F("  ----------------------------------------------"));
    Serial.println(F("    X-axis:             Y-axis:"));
    Serial.println(F("   'A'  'S'            'K'  'L'"));
    Serial.println(F("   <-    ->            <-    ->"));
    Serial.println(F("    Exit = 'E'"));

    // ----- flush the buffer
    while (Serial.available() > 0) Serial.read();

    // ----- control motors with 'A', 'S', 'K', and 'L' keys

    char keystroke = ' ';
    while (keystroke != 'E') {  //press 'E' key to exit

      // ----- check for keypress
      if (Serial.available() > 0) {
        keystroke = (char) Serial.read();
      }

      // ----- select task
      switch (keystroke) {
        case 'a':
        case 'A': {
            // ----- rotate motor1 CW
            for (step = 0; step < steps; step++) {
              step1_cw();
            }
            keystroke = ' ';    //otherwise motor will continue to rotate
            break;
          }
        case 's':
        case 'S': {
            // ------ rotate motor1 CCW
            for (step = 0; step < steps; step++) {
              step1_ccw();
            }
            keystroke = ' ';
            break;
          }
        case 'k':
        case 'K': {
            // ----- rotate motor2 CW
            for (step = 0; step < steps; step++) {
              step2_cw();
            }
            keystroke = ' ';
            break;
          }
        case 'l':
        case 'L': {
            // ----- rotate motor2 CCW
            for (step = 0; step < steps; step++) {
              step2_ccw();
            }
            keystroke = ' ';
            break;
          }
        case 'e':
        case 'E': {
            // ----- exit
            Serial.println(F(" "));
            Serial.println(F("  Calibration complete ..."));
            keystroke = 'E';
            break;
          }
        // ----- default for keystroke
        default: {
            break;
          }
      }
    }

    // ----- calculate steps to reach (0,0)
    calculate_steps(0,0);
  }

  // ----------------------------------
  // T2   set scale factor
  // ----------------------------------
  if (INPUT_STRING.startsWith("T2")) {
    Serial.println("T2");

    START = INPUT_STRING.indexOf('S');
    if (!(START < 0)) {
      FINISH = START + 6;
      SUB_STRING = INPUT_STRING.substring(START + 1, FINISH);
      SCALE_FACTOR = SUB_STRING.toFloat();
      Serial.print(F("Drawing now ")); Serial.print(SCALE_FACTOR * 100); Serial.println(F("%"));
    }
    else {
      Serial.println(F("Invalid scale factor ... try again. (1 = 100%)"));
    }
  }

  // ----------------------------------
  // T3   pen up
  // ----------------------------------
  if (INPUT_STRING.startsWith("T3")) {
    pen_up();
  }

  // ----------------------------------
  // T4   pen down
  // ----------------------------------
  if (INPUT_STRING.startsWith("T4")) {
    pen_down();
  }

  // ----------------------------------
  // T5   ABC test pattern
  // ----------------------------------
  if (INPUT_STRING.startsWith("T5")) {
    abc();
  }

  // ----------------------------------
  // T6   target test pattern
  // ----------------------------------
  if (INPUT_STRING.startsWith("T6")) {
    target();
  }

  // ----------------------------------
  // T7   radial line test pattern
  // ----------------------------------
  if (INPUT_STRING.startsWith("T7")) {
    radials();
  }
}

//----------------------------------------------------------------------------------------
// CALCULATE STEPS
// This routine calculates the the number of motor steps to reach (x,y) from 12 o'clock
//----------------------------------------------------------------------------------------
void calculate_steps(float x, float y) {

  // ----- locals
  float
  distance1,            //pen distance to motor1
  distance2,            //pen distance to motor2
  angle1,               //motor1 angle
  angle2;               //motor2 angle

  // ----- calculate distances
  distance1 = sqrt((OFFSET1-x)*(OFFSET1-x) + (YAXIS-y)*(YAXIS-y));
  distance2 = sqrt((OFFSET2-x)*(OFFSET2-x) + (YAXIS-y)*(YAXIS-y));

  // ----- calculate motor1 angle when pen at (x,y)
  if (x>OFFSET1) {
    angle1 = PI + acos(distance1/(2*LENGTH)) - atan((x-OFFSET1)/(YAXIS-y));       //radians
  } else {
    angle1 = PI + acos(distance1/(2*LENGTH)) + atan((OFFSET1-x)/(YAXIS-y));       //radians
  }

  // ----- calculate motor2 angle when pen at start position (0,0)
  if (x > OFFSET2) {
    angle2 = PI - acos(distance2/(2*LENGTH)) - atan((x-OFFSET2)/(YAXIS-y));       //radians
  } else {
    angle2 = PI - acos(distance2/(2*LENGTH)) + atan((OFFSET2-x)/(YAXIS-y));       //radians
  }

  // ----- calculate steps required to reach (x,y) from 12 o'clock
  STEPS1 = round(angle1*RAD_TO_DEG*STEPS_PER_DEG);
  STEPS2 = round(angle2*RAD_TO_DEG*STEPS_PER_DEG);
}

//----------------------------------------------------------------------------------------
// MOVE_TO
//----------------------------------------------------------------------------------------
void move_to(float X, float Y ) {

   // ----- apply scale factor
  float 
    x = X*SCALE_FACTOR,
    y = Y*SCALE_FACTOR;
  
 // ----- motor step counters
  long
    current_steps1 = STEPS1,            //get current motor steps
    current_steps2 = STEPS2,    
    this_steps1,                        //calculate the steps to reach this co-ordinate
    this_steps2,
    steps1,                             //extra motor steps to reach this co-ordinate
    steps2;

  bool
    direction1,                         //motor directions
    direction2;

  // ----- motor timers
  long
    current_time,         //system time
    previous_time1,       //previous system time for motor1
    previous_time2;       //previous system time for motor2

  // ----- calculate steps required to reach this position
  calculate_steps(x,y); 
  this_steps1 = STEPS1;
  this_steps2 = STEPS2;
  steps1 = abs(current_steps1-this_steps1);         //extra steps required
  steps2 = abs(current_steps2-this_steps2);         //extra steps required

  // ----- calculate the motor directions
  direction1 = (this_steps1>current_steps1)?CW:CCW;
  direction2 = (this_steps2>current_steps2)?CW:CCW;

  // ----- calculate motor delays
  calculate_delays(steps1,steps2);

  // ----- preload the timers and counters
  previous_time1 = micros();                      //reset the timer
  previous_time2 = micros();                      //reset the timer


  // ----- now step the motors
  while ((steps1 != 0) || (steps2 != 0)) {        //stop when both counters equal zero
    // ----- step motor1
    if (steps1 > 0) {                             //prevent additional step ... it occasionally happens!
      current_time = micros();
      if (current_time - previous_time1 > DELAY1) {
        previous_time1 = current_time;              //reset timer
        steps1--;                                   //decrement counter1
        step_motor1(direction1);
      }
    }    
    // ----- step motor2
    if (steps2 > 0) {                             //prevent additional step ... it occasionally happens!
      current_time = micros();
      if (current_time - previous_time2 > DELAY2) {
        previous_time2 = current_time;              //reset timer
        steps2--;                                   //decrement counter2
        step_motor2(direction2);
      }
    }
  }
}

//---------------------------------------------------------------------------
// CALCULATE DELAYS
// assigns values to DELAY1, DELAY2 ready for next pen move(s)
//---------------------------------------------------------------------------
void calculate_delays(long steps1, long steps2) {

  // ----- locals (globals capitalised)
  float rotate_time;

  long
  min_steps,
  max_steps,
  delay_max;

  // ----- find max and min number of steps
  max_steps = max(steps1, steps2);
  min_steps = min(steps1, steps2);

  // ----- error prevention
  if (max_steps < 1) max_steps = 1;   //prevent zero length delay
  if (min_steps < 1) min_steps = 1;   //prevent possible divide by zero

  // ----- calculate the total time for to complete the move
  rotate_time = (float)(max_steps * DELAY_MIN);

  // ----- calculate delay for motor with min_steps
  delay_max = (long)(rotate_time / ((float)min_steps));

  // ----- assign delays to each motor
  DELAY1 = (steps1 > steps2) ? DELAY_MIN : delay_max;
  DELAY2 = (steps1 > steps2) ? delay_max : DELAY_MIN;
}

//--------------------------------------------------------------------
// STEP MOTOR1
//--------- -----------------------------------------------------------
void step_motor1(bool dir1) {
  if (dir1==CW){
    step1_cw();
  } else {
    step1_ccw();
  }  
}

//--------------------------------------------------------------------
// STEP MOTOR2 
//--------- -----------------------------------------------------------
void step_motor2(bool dir2) {
  if (dir2==CW){
    step2_cw();
  } else {
    step2_ccw();
  }  
}

//--------------------------------------------------------------------
// STEP1_CW        (step actuator motor1 clockwise)
//--------- -----------------------------------------------------------
void step1_cw() {
  
  //----- set direction
  digitalWrite(DIR1,CW);
  delayMicroseconds(PULSE_WIDTH);

  //----- step motor
  digitalWrite(STEP1,HIGH);  
  delayMicroseconds(PULSE_WIDTH);
  digitalWrite(STEP1,LOW);  
  delayMicroseconds(PULSE_WIDTH);
}

//--------------------------------------------------------------------
// STEP1_CCW        (step actuator motor1 counter-clockwise)
//--------- -----------------------------------------------------------
void step1_ccw() {
  
  //----- set direction
  digitalWrite(DIR1,CCW);
  delayMicroseconds(PULSE_WIDTH);

  //----- step motor
  digitalWrite(STEP1,HIGH);  
  delayMicroseconds(PULSE_WIDTH);
  digitalWrite(STEP1,LOW);  
  delayMicroseconds(PULSE_WIDTH);
}

//--------------------------------------------------------------------
// STEP2_CW        (step actuator motor2 clockwise)
//--------- -----------------------------------------------------------
void step2_cw() {

  //----- set direction
  digitalWrite(DIR2,CW);
  delayMicroseconds(PULSE_WIDTH);

  //----- step motor
  digitalWrite(STEP2,HIGH);  
  delayMicroseconds(PULSE_WIDTH);
  digitalWrite(STEP2,LOW);  
  delayMicroseconds(PULSE_WIDTH);
}

//--------------------------------------------------------------------
// STEP2_CCW        (step actuator motor2 counter-clockwise)
//--------- -----------------------------------------------------------
void step2_ccw() {

  //----- set direction
  digitalWrite(DIR2,CCW);
  delayMicroseconds(PULSE_WIDTH);

  //----- step motor
  digitalWrite(STEP2,HIGH);  
  delayMicroseconds(PULSE_WIDTH);
  digitalWrite(STEP2,LOW);  
  delayMicroseconds(PULSE_WIDTH);
}

//---------------------------------------------------------------------------
// PEN_UP
// Raise the pen
// Changing the value in OCR2B changes the pulse-width to the SG-90 servo
//---------------------------------------------------------------------------
void pen_up() {
  OCR2B = 148;                //1mS pulse
  delay(100);                 //give pen-lift time to respond
}

//---------------------------------------------------------------------------
// PEN_DOWN
// Lower the pen
// Changing the value in OCR2B changes the pulse-width to the SG-90 servo
//---------------------------------------------------------------------------
void pen_down() {
  OCR2B = 140;                //2mS pulse
  delay(100);                 //give pen-lift time to respond
}

//----------------------------------------------------------------------------
// ABC test pattern
//----------------------------------------------------------------------------
void abc() {
  // ------ letter C
  pen_up();
  move_to(71.607584, 21.035879);
  pen_down();
  move_to(70.163261, 20.392706);
  move_to(68.665833, 19.931058);
  move_to(67.123530, 19.654409);
  move_to(65.471170, 19.558347);
  move_to(60.824111, 20.352237);
  move_to(57.604313, 22.327055);
  move_to(55.521083, 25.458294);
  move_to(54.702496, 29.861135);
  move_to(55.523176, 34.275230);
  move_to(57.604313, 37.395213);
  move_to(60.826497, 39.380152);
  move_to(65.471170, 40.177231);
  move_to(67.123530, 40.081169);
  move_to(68.665833, 39.804521);
  move_to(70.163261, 39.342872);
  move_to(71.607584, 38.699700);
  move_to(71.607584, 34.586572);
  move_to(70.133934, 35.457974);
  move_to(68.798946, 36.010859);
  move_to(67.396000, 36.346751);
  move_to(65.883816, 36.463436);
  move_to(63.362672, 35.969576);
  move_to(61.571020, 34.706372);
  move_to(60.460591, 32.773568);
  move_to(60.000312, 29.861135);
  move_to(60.459740, 26.961684);
  move_to(61.571020, 25.029205);
  move_to(63.362672, 23.766000);
  move_to(65.883816, 23.272141);
  move_to(67.396000, 23.388826);
  move_to(68.798946, 23.724718);
  move_to(70.133934, 24.277603);
  move_to(71.607584, 25.149006);
  move_to(71.607584, 21.035879);
  pen_up();

  // ------ top inside-loop in letter 'B'
  pen_up();
  move_to(43.041974, 32.124019);
  pen_down();
  move_to(44.193140, 32.287491);
  move_to(44.878907, 32.656463);
  move_to(45.321578, 33.273441);
  move_to(45.504526, 34.227172);
  move_to(45.322608, 35.168069);
  move_to(44.878907, 35.784570);
  move_to(44.190670, 36.163294);
  move_to(43.041974, 36.330325);
  move_to(40.206713, 36.330325);
  move_to(40.206713, 32.124019);
  move_to(43.041974, 32.124019);
  pen_up();

  // ----- bottom inside-loop in letter 'B'
  pen_up();
  move_to(43.215018, 23.431875);
  pen_down();
  move_to(44.684832, 23.634884);
  move_to(45.531148, 24.084119);
  move_to(46.084429, 24.845298);
  move_to(46.316505, 26.054160);
  move_to(46.088504, 27.238072);
  move_to(45.544461, 27.984270);
  move_to(44.697894, 28.432828);
  move_to(43.215018, 28.636513);
  move_to(40.206713, 28.636513);
  move_to(40.206713, 23.431875);
  move_to(43.215018, 23.431875);
  pen_up();

  // ----- outside of letter 'B'
  pen_up();
  move_to(47.980391, 30.579932);
  pen_down();
  move_to(49.467494, 29.872216);
  move_to(50.536123, 28.809558);
  move_to(51.189538, 27.438932);
  move_to(51.441274, 25.641517);
  move_to(50.881551, 23.051631);
  move_to(49.497855, 21.355344);
  move_to(47.408388, 20.394118);
  move_to(43.587730, 19.944368);
  move_to(35.081941, 19.944368);
  move_to(35.081941, 39.817832);
  move_to(42.775754, 39.817832);
  move_to(46.788467, 39.403201);
  move_to(48.765745, 38.566589);
  move_to(50.084134, 37.024736);
  move_to(50.629298, 34.559950);
  move_to(50.441596, 33.165564);
  move_to(49.950432, 32.084086);
  move_to(49.146555, 31.229561);
  move_to(47.980391, 30.579932);
  pen_up();

  // ----- outside of letter 'A'
  pen_up();
  move_to(26.057020, 23.564986);
  pen_down();
  move_to(18.043741, 23.564986);
  move_to(16.779187, 19.944368);
  move_to(11.627794, 19.944368);
  move_to(18.988829, 39.817832);
  move_to(25.098621, 39.817832);
  move_to(32.459656, 19.944368);
  move_to(27.308262, 19.944368);
  move_to(26.057020, 23.564986);
  pen_up();

  // ----- inside of letter 'A'
  pen_up();
  move_to(19.321606, 27.252160);
  pen_down();
  move_to(24.765843, 27.252160);
  move_to(22.050380, 35.158949);
  move_to(19.321606, 27.252160);
  pen_up();

  // ----- home
  move_to(0.0000, 0.0000);
}


//----------------------------------------------------------------------------
// TARGET test pattern
//----------------------------------------------------------------------------
void target() {

  // ----- circle
  pen_up();
  move_to(130,100);
  pen_down();
  move_to(128,88);
  move_to(122,79);
  move_to(112,73);
  move_to(100,70);
  move_to(87,73);
  move_to(78,79);
  move_to(71,88);
  move_to(69,100);
  move_to(71,111);
  move_to(78,123);
  move_to(87,130);
  move_to(100,132);
  move_to(112,130);
  move_to(122,123);
  move_to(129,110);
  move_to(130,100);
  pen_up();

  // ----- back-slash
  pen_up();
  move_to(50,150);
  pen_down();
  move_to(78,123);
  move_to(100,100);
  move_to(123,79);
  move_to(150,50);
  pen_up();

  // ----- slash
  pen_up();
  move_to(50,50);
  pen_down();
  move_to(78,79);
  move_to(100,100);
  move_to(122,123);
  move_to(150,150);
  pen_up();

  // ----- square
  pen_up();
  move_to(50,150);
  pen_down();
  move_to(100,150);
  move_to(150,150);
  move_to(150,100);
  move_to(150,50);
  move_to(100,50);
  move_to(50,50);
  move_to(50,100);
  move_to(50,150);
  pen_up();

  // ------ home
  move_to(0.0000, 0.0000);
}

//----------------------------------------------------------------------------
// RADIALS test pattern
//----------------------------------------------------------------------------
void radials() {

  // ----- move to the centre of the square
  pen_up();
  move_to(100, 100);

  // ----- draw octant 0 radials
  pen_down();
  move_to(150, 100);
  pen_up();
  move_to(100, 100);

  pen_down();
  move_to(150, 125);
  pen_up();
  move_to(100, 100);

  pen_down();
  move_to(150, 150);
  pen_up();
  move_to(100, 100);

  // ----- draw octant 1 radials
  pen_down();
  move_to(125, 150);
  pen_up();
  move_to(100, 100);

  pen_down();
  move_to(100, 150);
  pen_up();
  move_to(100, 100);

  // ----- draw octant 2 radials
  pen_down();
  move_to(75, 150);
  pen_up();
  move_to(100, 100);

  pen_down();
  move_to(50, 150);
  pen_up();
  move_to(100, 100);

  // ----- draw octant 3 radials
  pen_down();
  move_to(50, 125);
  pen_up();
  move_to(100, 100);

  pen_down();
  move_to(50, 100);
  pen_up();
  move_to(100, 100);

  // ----- draw octant 4 radials
  pen_down();
  move_to(50, 75);
  pen_up();
  move_to(100, 100);

  pen_down();
  move_to(50, 50);
  pen_up();
  move_to(100, 100);

  // ----- draw octant 5 radials
  pen_down();
  move_to(75, 50);
  pen_up();
  move_to(100, 100);

  pen_down();
  move_to(100, 50);
  pen_up();
  move_to(100, 100);

  // ----- draw octant 6 radials
  pen_down();
  move_to(125, 50);
  pen_up();
  move_to(100, 100);

  pen_down();
  move_to(150, 50);
  pen_up();
  move_to(100, 100);

  // ----- draw octant 7 radials
  pen_down();
  move_to(150, 75);
  pen_up();
  move_to(100, 100);
  pen_up();

  // ----- draw box
  move_to(50, 50);
  pen_down();
  move_to(50, 150);
  move_to(150, 150);
  move_to(150, 50);
  move_to(50, 50);
  pen_up();

  // home --------------
  move_to(0.0000, 0.0000);
}


