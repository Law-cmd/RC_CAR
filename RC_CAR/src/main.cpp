#include <Arduino.h>

#define MOTOR_Right_Speed 10
#define MOTOR_Left_Speed  11
#define MOTOR_Right_Forward 4
#define MOTOR_Right_Backward  5
#define MOTOR_Left_Forward  6
#define MOTOR_Left_Backward  7
struct SensorData 
{
  uint16_t front, right, left;
};

SensorData readSensor();
void setup() 
{
  Serial.begin(9600);
  pinMode(MOTOR_Right_Speed, OUTPUT);
  pinMode(MOTOR_Left_Speed, OUTPUT);
  pinMode(MOTOR_Right_Forward, OUTPUT);
  pinMode(MOTOR_Right_Backward, OUTPUT);
  pinMode(MOTOR_Left_Forward, OUTPUT);
  pinMode(MOTOR_Left_Backward, OUTPUT);

  analogWrite(MOTOR_Right_Speed, 200);
  analogWrite(MOTOR_Left_Speed, 200);
}

void loop() 
{
  SensorData data = readSensor();
  delay(100);
  if(data.front < 100)
  {
    digitalWrite(MOTOR_Right_Forward, LOW);
    digitalWrite(MOTOR_Right_Backward, LOW);
    digitalWrite(MOTOR_Left_Forward, HIGH);
    digitalWrite(MOTOR_Left_Backward, LOW);
  }else
  {  
    digitalWrite(MOTOR_Right_Forward, HIGH);
    digitalWrite(MOTOR_Right_Backward, LOW); 
    digitalWrite(MOTOR_Left_Forward, LOW);
    digitalWrite(MOTOR_Left_Backward, HIGH);
  }
}



SensorData readSensor() 
{
  SensorData s;

  uint16_t raw_front = analogRead(A3);
  uint16_t raw_right = analogRead(A4);
  uint16_t raw_left  = analogRead(A5);

  s.front = (uint16_t)((18879.80769f / (raw_front + 45.205570290f)) - 15);
  s.right = (uint16_t)((22628.57143f / (raw_right + 60.71428571f))  - 30);
  s.left  = (uint16_t)((18900.57143f / (raw_left  + 60.71428571f))  - 30);

  
  if      (s.front > 150) s.front = 151;
  else if (s.front < 20)  s.front = 19;

  if      (s.right > 80)  s.right = 81;
  else if (s.right < 10)  s.right = 9;

  if      (s.left > 80)   s.left = 81;
  else if (s.left < 10)   s.left = 9;

  
  Serial.print(" Vorne: ");  Serial.print(s.front);
  Serial.print(" Rechts: "); Serial.print(s.right);
  Serial.print(" Links: ");  Serial.print(s.left);
  int16_t diff = s.right - s.left;
  Serial.print(" Differenz: "); Serial.print(diff);
  Serial.println();

  return s;
}