#include <Arduino.h>

// ===== PIN-DEFINITIONEN =====
#define MOTOR_Right_Speed     10   // PWM
#define MOTOR_Left_Speed      11   // PWM
#define MOTOR_Right_Forward   4
#define MOTOR_Right_Backward  5
#define MOTOR_Left_Forward    6
#define MOTOR_Left_Backward   7

#define START_Button  3
#define STOP_Button   2

#define IR_FRONT  A3
#define IR_RIGHT  A4
#define IR_LEFT   A5

// ===== KALIBRIERPARAMETER =====
const float PARAM_M_FRONT = 18879.80769f;
const float PARAM_D_FRONT = 45.20557f;
const float PARAM_K_FRONT = 15.0f;

const float PARAM_M_RIGHT = 22628.57143f;
const float PARAM_D_RIGHT = 60.71428571f;
const float PARAM_K_RIGHT = 30.0f;

const float PARAM_M_LEFT  = 17500.57143f;
const float PARAM_D_LEFT  = 60.71428571f;
const float PARAM_K_LEFT  = 30.0f;

// ===== SCHWELLWERTE =====
const uint16_t FRONT_STOP  = 50;   // cm
const uint16_t SIDE_STOP   = 25;   // cm

// ===== GESCHWINDIGKEIT =====
const uint8_t BASE_SPEED = 200;

// ===== KREIS-ERKENNUNG =====
const uint16_t TURN_RESET_TIME = 2200;  // ms — Zähler reset wenn kein Turn in dieser Zeit

// ===== STATE MACHINE =====
/*
 * Zustandsübergänge:
 *   STOPP   --[Start-Button]-------------------------> FORWARD
 *   FORWARD --[Stop-Button]-------------------------> STOPP
 *   FORWARD --[front<50 || right<25 || left<25]-----> TURNING
 *   TURNING --[kein Hindernis mehr]-----------------> FORWARD
 *   TURNING --[Stop-Button]-------------------------> STOPP
 */
enum State { STOPP, FORWARD, TURNING };

// ===== GLOBALE VARIABLEN =====
State state = STOPP;

uint16_t ir_front = 70, ir_right = 35, ir_left = 35;
uint16_t ir_front_prev = 70, ir_right_prev = 35, ir_left_prev = 35;

bool    turn_left_flag   = false;
uint8_t turn_count_right = 0;
uint8_t turn_count_left  = 0;

unsigned long last_turn_millis  = 0;
unsigned long prev_millis_20ms  = 0;
unsigned long prev_millis_100ms = 0;


// ===== FUNKTIONSPROTOTYPEN =====
void checkButtons();
void readSensors();
void processStateMachine();
void driveForward();
void doTurn();
void forward();
void turnRight();
void turnLeft();
void backward();
void stop();
void printStatus();


// ===== SETUP =====
void setup() {
  Serial.begin(9600);

  pinMode(MOTOR_Right_Speed,    OUTPUT);
  pinMode(MOTOR_Left_Speed,     OUTPUT);
  pinMode(MOTOR_Right_Forward,  OUTPUT);
  pinMode(MOTOR_Right_Backward, OUTPUT);
  pinMode(MOTOR_Left_Forward,   OUTPUT);
  pinMode(MOTOR_Left_Backward,  OUTPUT);

  pinMode(START_Button, INPUT_PULLUP);
  pinMode(STOP_Button,  INPUT_PULLUP);

  analogWrite(MOTOR_Right_Speed, BASE_SPEED);
  analogWrite(MOTOR_Left_Speed,  BASE_SPEED);

  stop();
  Serial.println("Bereit. Start-Taste druecken.");
}


// ===== LOOP =====
void loop() {
  checkButtons();

  if (millis() - prev_millis_20ms >= 20) {
    prev_millis_20ms = millis();
    readSensors();
    processStateMachine();
  }

  if (millis() - prev_millis_100ms >= 100) {
    prev_millis_100ms = millis();
    printStatus();
  }
}


// ===== SENSOREN =====
void readSensors() {
  uint16_t raw, new_val;

  ir_front_prev = ir_front;
  ir_right_prev = ir_right;
  ir_left_prev  = ir_left;

  // Vorne
  raw     = analogRead(IR_FRONT);
  new_val = (uint16_t)(PARAM_M_FRONT / (raw + PARAM_D_FRONT) - PARAM_K_FRONT);
  if      (new_val > 150) new_val = 151;
  else if (new_val < 20)  new_val = 19;
  ir_front = (new_val + ir_front_prev) / 2;

  // Rechts
  raw     = analogRead(IR_RIGHT);
  new_val = (uint16_t)(PARAM_M_RIGHT / (raw + PARAM_D_RIGHT) - PARAM_K_RIGHT);
  if      (new_val > 80) new_val = 81;
  else if (new_val < 10) new_val = 9;
  ir_right = (new_val + ir_right_prev) / 2;

  // Links
  raw     = analogRead(IR_LEFT);
  new_val = (uint16_t)(PARAM_M_LEFT / (raw + PARAM_D_LEFT) - PARAM_K_LEFT);
  if      (new_val > 80) new_val = 81;
  else if (new_val < 10) new_val = 9;
  ir_left = (new_val + ir_left_prev) / 2;
}


// ===== STATE MACHINE =====
void processStateMachine() {
  switch (state) {
    case STOPP:   stop();         break;
    case FORWARD: driveForward(); break;
    case TURNING: doTurn();       break;
  }
}

void driveForward() {
  forward();

  // Zähler zurücksetzen wenn lange genug geradeaus gefahren
  if (millis() - last_turn_millis >= TURN_RESET_TIME) {
    turn_count_right = 0;
    turn_count_left  = 0;
  }

  if (ir_front < FRONT_STOP || ir_right < SIDE_STOP || ir_left < SIDE_STOP) {
    int16_t diff = (int16_t)ir_right - (int16_t)ir_left;
    turn_left_flag = (diff > 0);

    
    if (turn_count_right >= 4) {
      turn_left_flag = true;
      turn_count_right = 0;
    } else if (turn_count_left >= 4) {
      turn_left_flag = false;
      turn_count_left = 0;
    }

    if (turn_left_flag) turn_count_left++;
    else                turn_count_right++;

    last_turn_millis = millis();
    state = TURNING;
  }
}

void doTurn() 
{
  if (turn_left_flag) {
    turnLeft();
  } else {
    turnRight();
  }

  // Wieder vorwärts wenn Weg frei
  if (ir_front >= FRONT_STOP && ir_right >= SIDE_STOP && ir_left >= SIDE_STOP) {
    state = FORWARD;
  }
}


// ===== BUTTONS =====
void checkButtons() 
{
  if (digitalRead(START_Button) == LOW && state == STOPP)
    state = FORWARD;
  if (digitalRead(STOP_Button) == LOW)
    state = STOPP;
}


// ===== MOTORSTEUERUNG =====
void forward() 
{
  digitalWrite(MOTOR_Right_Forward, HIGH);
  digitalWrite(MOTOR_Right_Backward, LOW);
  digitalWrite(MOTOR_Left_Forward, LOW);
  digitalWrite(MOTOR_Left_Backward, HIGH);
}

void backward()
{
  digitalWrite(MOTOR_Right_Forward, LOW);
  digitalWrite(MOTOR_Right_Backward, HIGH);
  digitalWrite(MOTOR_Left_Forward, HIGH);
  digitalWrite(MOTOR_Left_Backward, LOW);
}

void turnRight()
{
  digitalWrite(MOTOR_Right_Forward, LOW);
  digitalWrite(MOTOR_Right_Backward, HIGH);
  digitalWrite(MOTOR_Left_Forward, LOW);
  digitalWrite(MOTOR_Left_Backward, HIGH);
   analogWrite(MOTOR_Right_Speed, 120);
  analogWrite(MOTOR_Left_Speed,  120);
}

void turnLeft() 
{
  digitalWrite(MOTOR_Right_Forward, HIGH);
  digitalWrite(MOTOR_Right_Backward, LOW);
  digitalWrite(MOTOR_Left_Forward, HIGH);
  digitalWrite(MOTOR_Left_Backward, LOW); 
  analogWrite(MOTOR_Right_Speed, 120);
  analogWrite(MOTOR_Left_Speed,  120);
}

void stop() {
  digitalWrite(MOTOR_Right_Forward, LOW);
  digitalWrite(MOTOR_Right_Backward, LOW);
  digitalWrite(MOTOR_Left_Forward, LOW);
  digitalWrite(MOTOR_Left_Backward, LOW);
}


// ===== SERIAL OUTPUT =====
void printStatus() {
  Serial.print("F: ");   Serial.print(ir_front);
  Serial.print("\tR: "); Serial.print(ir_right);
  Serial.print("\tL: "); Serial.print(ir_left);
  Serial.print("\tDiff: "); Serial.print((int16_t)(ir_right - ir_left));
  Serial.print("  TR: "); Serial.print(turn_count_right);
  Serial.print("  TL: "); Serial.print(turn_count_left);
  Serial.print("  [");
  switch (state) {
    case STOPP:   Serial.print("STOPP");   break;
    case FORWARD: Serial.print("FORWARD"); break;
    case TURNING: Serial.print("TURNING"); break;
  }
  Serial.println("]");
}