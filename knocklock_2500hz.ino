#include <basicMPU6050.h>
#include <Servo.h>
#include "math.h"

// Create instance
basicMPU6050<> imu;
Servo servo;

//a must be between 0 and 1
double EMA(double a, double x, double y) {
  return (a*x + (1-a)*y);
}

float knock_data[4][2]; //element 0 = amplitude, element 1 = time
int index = 0;

//can store a max of 10 knocks - hence passcode cannot exceed 10 locks in length
float KNOCK_AMPLITUDE[4] = {1550, 1550, 1550, 1550};
float knock_amplitude_error;
int KNOCK_TIME[4] = {0, 300, 300}; //all knock time arrays must start with 0 | each time represents the time a knock occurs after the previous knock
int knock_time_error;

void setupKnocklock(float amplitude_error, int time_error) {
  knock_amplitude_error = amplitude_error;
  knock_time_error = time_error;
}

bool correct1 = true, correct2 = true;
int correctCode = 0;
bool openDoor = false;
void addKnock(float output_peak, int time) {
  knock_data[index][0] = output_peak;
  knock_data[index][1] = time;
  index++;
  if (index >= 3) {
    if (checkKnockPattern()) {
      correctCode++;
      openDoor = true;
    }
    index = 0;
    //delay(5000);
    correct1 = true;
    correct2 = true;
  }
}

bool checkKnockPattern() {
  bool correct = true;
  for (int i = 0; i < 3; i++) {
    if (correct) {
      if (abs(knock_data[i][0]-KNOCK_AMPLITUDE[i]) > knock_amplitude_error) {
        correct = false;
        correct1 = false;
      }
      if (i > 0) {
        if (abs(abs(knock_data[i][1]-knock_data[i-1][1])-KNOCK_TIME[i]) > knock_time_error) {
          correct = false;
          correct2 = false;
        }
      }
    }
    Serial.print( i );
    Serial.print(" ");
    Serial.print( correct1 );
    Serial.print(" ");
    Serial.print( correct2 );
    Serial.print(" ");
    Serial.print( correctCode );
    Serial.print(" ");
    Serial.print( abs(knock_data[i][0]-KNOCK_AMPLITUDE[i]) );
    Serial.print(" ");
    Serial.print( abs(abs(knock_data[i][1]-knock_data[i-1][1])-KNOCK_TIME[i]) );
    Serial.println();
  }
  return correct;
}

void setup() {
  imu.setup(); // Set registers - Always required
  servo.attach(9);
  setupKnocklock(800, 100); //amp error, time error
  Serial.begin(19200); // Start console
}


float xn = 0, xn1 = 0;
float yn = 0, yn1 = 0;
float zn = 0, zn1 = 0;

int count = 0;
int startTime = 0;
int openTime = 0;
int state = 0;

float peak_ampl = 0;
int peak_time = 0;

void loop() {

  int sample_rate = 2500;

  xn = imu.rawAz();
  yn = 0.987512*yn1 + 0.00624395*xn + 0.00624395*xn1;
  zn = 0.77673*zn1 + 0.1116352*xn + 0.1116352*xn1;

  delay((1000.0 / sample_rate));
  xn1 = xn;
  yn1 = yn;
  zn1 = zn;

  float output = abs(xn - zn);
  
  switch(state) {
    case 0:
      if (output >= 1055) {
        state = 1;
        if (output > peak_ampl) peak_ampl = output;
      }
      break;
    case 1:
      if (output > peak_ampl) peak_ampl = output;
      count++;
      state = 2;
      startTime = millis();
      break;
    case 2: //account for resonant vibration in material
      if (output > peak_ampl) peak_ampl = output;
      if (millis()-startTime > 100 && output <= 3) {

        addKnock(peak_ampl, startTime);

        state = 0;
        peak_ampl = 0;

      }
      break;
  }

  if (openDoor) {
    servo.write(180);
    delay(2000);
    openDoor = false;
  } else {
    servo.write(90);
  }

  //Serial.print( xn );
  //Serial.print(" ");
  /*
  Serial.print( yn );
  Serial.print(" ");
  Serial.print( zn );
  Serial.print(" ");
  Serial.print( output );
  Serial.print(" ");
  */
  //Serial.print( output );
  //Serial.print(" ");
  /*
  Serial.print( index );
  Serial.print(" ");
  Serial.print( (millis() - openTime) % 1000 );
  Serial.print(" ");
  Serial.print( openDoor );
  Serial.print(" ");
  Serial.print( correctCode );
  Serial.println();
  */
}