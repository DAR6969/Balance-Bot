/*
  Encoders.h - A Library for Encoders
  Created by Shivam Malviya, Jan 30, 2020.
*/
#include "Arduino.h"
#include "Motor.h"

Motor::Motor(byte M1, byte M2, byte EN, byte A, byte B) {
    this->M1 = M1;
    this->M2 = M2;
    this->EN = EN;
    this->encoder.A = A;
    this->encoder.B = B;

    init();
}

void Motor::init() {
    pinMode(M1, OUTPUT);
    pinMode(M2, OUTPUT);
    pinMode(EN, OUTPUT);
        
    stop(); 
}

void Motor::stop() {
    digitalWrite(M1, LOW);
    digitalWrite(M2, LOW);
    analogWrite(EN, 0);
}

void Motor::forward(int speed) {
    digitalWrite(M1, HIGH);
    digitalWrite(M2, LOW);
    analogWrite(EN, speed);
}

void Motor::reverse(int speed) {
    digitalWrite(M1, LOW);
    digitalWrite(M2, HIGH);
    analogWrite(EN, speed);
}

void Motor::generate(double voltage) {
    
    // Serial.print("Voltage : ");
    // Serial.println(voltage);
    // Limit the value of the voltage to +/- 12V
    voltage = (voltage > 12.0) ? 12.0 : voltage;
    voltage = (voltage < -12.0) ? -12.0 : voltage;
    // Serial.print("Voltage: ");Serial.println(voltage);
    int analogVal = (int) ((double)voltage*1023.0/12.0 + 0.5);
    // Serial.print(analogVal);
    if (analogVal > 0) {
        forward(analogVal);    
    }
    else {
        reverse(-analogVal);
    }
}

void leftEncoderInterruptA() {
    double dT = (double) ((micros() - leftMotor.encoder.timeStamp)) / 1e6;
    
    if (digitalReadFast(18) != digitalReadFast(17)) {
        leftMotor.encoder.rotation--;
        leftMotor.encoder.phiDot = -0.0116355283 / dT;
    } 
    else {
        leftMotor.encoder.rotation++;
        leftMotor.encoder.phiDot = 0.0116355283 / dT;
  
    }
    leftMotor.encoder.timeStamp = micros();
}

void rightEncoderInterruptA() { 
    double dT = (double) ((micros() - rightMotor.encoder.timeStamp)) / 1e6;
    
    if (digitalReadFast(3) != digitalReadFast(4)) {
        rightMotor.encoder.rotation++;
        rightMotor.encoder.phiDot = 0.0116355283 / dT;
    } 
    else {
        rightMotor.encoder.rotation--;
        rightMotor.encoder.phiDot = -0.0116355283 / dT;  
    }
    rightMotor.encoder.timeStamp = micros();
 
}

void attachMotorInterrupts() {
  attachInterrupt(
    digitalPinToInterrupt(leftMotor.encoder.A), 
    leftEncoderInterruptA,
    CHANGE
    );

  attachInterrupt(
    digitalPinToInterrupt(rightMotor.encoder.A), 
    rightEncoderInterruptA,
    CHANGE
    );
}