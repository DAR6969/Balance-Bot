#include "I2Cdev.h"
#include "i2c_lib.h"
#include "TimerInterrupts.h"
#include "MPU6050.h"
#include <Motor.h>

// Register addresses of mpu6050
#define ACCEL_YOUT_H 0x3D  
#define ACCEL_ZOUT_H 0x3F 
#define GYRO_XOUT_H 0x43

const int MPUAddress = 0x68 << 1; 

// Negative feedbackmatrix
double K[] = {0, 0, 0, 0};
 

int16_t ay, az, gx;
uint8_t tmpBytesArr2[2];

// Motor Pins
const byte lM1 = 22;  //M1 - OUT1 - IN1 - 22
const byte lM2 = 23;  //M2 - OUT2 - IN2 - 23
const byte lME = 11;  //ENA - 11
const byte lMA = 18;  //A - 18
const byte lMB = 17;  //B - 17

const byte rM1 = 30;  //M2 - OUT4 - IN4 - 30
const byte rM2 = 31;  //M1 - OUT3 - IN3 - 31
const byte rME = 12;  //ENB - 12
const byte rMA = 19;   //A - 19
const byte rMB = 16;   //B - 16


// Hyper params
const double fCut = 5.0;
const double alpha   = 0.05;
const double dT = 2e-3;
const double Tau = 1/(2*PI*fCut);
const double alphaHPF = Tau/(Tau+dT);
const double alphaLPF = Tau/(Tau+dT);

// variables used in ISRs
volatile double accFy, accFz, gyFx, gxVal, gxPrev;
volatile double pitch, pitchPrev, pitchRate; 
volatile unsigned long timeStampTimer5, currTimeTimer5, dT5;


// Declare mpu sensor object and motor objects
MPU6050 mpu;
Motor leftMotor(lM1, lM2, lME, lMA, lMB, 200);
Motor rightMotor(rM1, rM2, rME, rMA, rMB, 200);

void setup() {
  Serial.begin(115200);
  // Initialize the sensor and the motors
  initMPUSensor();
  attachMotorInterrupts();
  // Initialize timers
  initTimer3();
  initTimer4();
  initTimer5();
  // Allow all global interrupts
  sei(); 
}

void loop() {
  delay(5);
  // For debugging
  Serial.print("X: ");
  Serial.println(x());
  Serial.print("XDot: ");
  Serial.println(xDot());
  Serial.print("Pitch: ");
  Serial.println(pitch);  
  Serial.print("PitchRate: ");
  Serial.println(pitchRate);
}

// Timer-3 interrupts are used to calculate the angular velocity using encoders' readings
ISR(TIMER3_COMPA_vect) {
    leftMotor.encoder.phiDot = 0.01163552835 * (leftMotor.encoder.rotation - leftMotor.encoder.lastRotation) / SAMPLING_TIME; //0.01163552835 = 2 * M_P! / 540.0
    rightMotor.encoder.phiDot = 0.01163552835 * (rightMotor.encoder.rotation - rightMotor.encoder.lastRotation) / SAMPLING_TIME; //0.01163552835 = 2 * M_P! / 540.0
    leftMotor.encoder.lastRotation = leftMotor.encoder.rotation;
    rightMotor.encoder.lastRotation = rightMotor.encoder.rotation;        
}

// Apply torque to the motors according to the LQR system
ISR(TIMER5_COMPA_vect){
  double torque, voltage; 
  torque = -(K[0]*x() + K[1]*xDot() + K[2]*pitch*PI/180 + K[3]*pitchRate*PI/180);
  voltage = (RESISTANCE*torque/(MOTOR_CONSTANT*GEAR_RATIO)); //+ (xDot()/0.03)*GEAR_RATIO*MOTOR_CONSTANT;
  driveMotors(voltage);  
}

// Read data from the MPU-6050 sensor
ISR(TIMER4_COMPA_vect){
  check_status(i2c_read_multi_byte(MPUAddress, ACCEL_YOUT_H, 2, tmpBytesArr2));
  ay = ((int16_t)tmpBytesArr2[0] << 8) | tmpBytesArr2[1];
  check_status(i2c_read_multi_byte(MPUAddress, ACCEL_ZOUT_H, 2, tmpBytesArr2));
  az = ((int16_t)tmpBytesArr2[0] << 8) | tmpBytesArr2[1];
  check_status(i2c_read_multi_byte(MPUAddress, GYRO_XOUT_H, 2, tmpBytesArr2));
  gx = ((int16_t)tmpBytesArr2[0] << 8) | tmpBytesArr2[1];

  gxVal = (double)gx/131.0;
  
  accFy = alphaLPF*accFy+(1-alphaLPF)*ay;
  accFz = alphaLPF*accFz+(1-alphaLPF)*az;
  gyFx = (1-alphaHPF)*(gxVal-gxPrev)+(1-alphaHPF)*gyFx;

  pitchPrev = pitch;
  pitch = (1-alpha)*(dT*gyFx+pitch)-alpha*atan2(accFy, abs(accFz))*180/PI;
  pitchRate = (pitch-pitchPrev)/dT;
}

double x() {
  return leftMotor.encoder.getX();
}

double xDot() {
  return leftMotor.encoder.getXDot();
}

void check_status(STAT status){
  if(status != OK){
    Serial.print("ERROR: ");
    Serial.println(status);
  }
}

void initMPUSensor(void){
  
  Wire.begin();

  // initialize device
  Serial.println("Initializing I2C devices...");
  mpu.initialize();
  
  // verify connection
  Serial.println("Testing device connections...");
  Serial.println(mpu.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");

  Serial.println("Updating internal sensor offsets...");

  mpu.setXGyroOffset(13);
  mpu.setYGyroOffset(47);
  mpu.setZGyroOffset(-916);

  mpu.setXAccelOffset(-3768);
  mpu.setYAccelOffset(-1314);
  mpu.setZAccelOffset(1419);

  Serial.print(mpu.getXAccelOffset());Serial.print("\t");
  Serial.print(mpu.getYAccelOffset());Serial.print("\t");
  Serial.print(mpu.getZAccelOffset());Serial.print("\t");
  Serial.print(mpu.getXGyroOffset());Serial.print("\t");
  Serial.print(mpu.getYGyroOffset());Serial.print("\t");
  Serial.print(mpu.getZGyroOffset());Serial.print("\t");Serial.print("\n");
  
  i2c_init();
  Serial.println("I2C Reset");
  Serial.println(TWSR, HEX);
  pitch = accFy = accFz = gxPrev = gyFx = 0;
}
