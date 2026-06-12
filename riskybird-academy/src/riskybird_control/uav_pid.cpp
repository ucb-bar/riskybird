#include "uav_pid.h"
#include <stdint.h>
#include <Arduino.h>

float getAltitude(float tofDistance, float roll, float pitch) {
  // roll and pitch angles are assumed to be in radians
  // tofDistance is the raw measurement from the ToF sensor

  // cos(theta) ~= 1 - theta^2 / 2 for small theta
  // we compute 1 / cos(theta) to correct the ToF measurement
  float rollCorrection = 1.0 / (1.0 - roll * roll / 2.0);
  float pitchCorrection = 1.0 / (1.0 - pitch * pitch / 2.0);

  // the actual altitude is the ToF measurement divided by the correction factors
  float altitude = tofDistance * rollCorrection * pitchCorrection;

  return altitude;
}

UAV_PID::UAV_PID(void) {

    base_speed = 0.5;

    Kp_roll = 1000;
    Kd_roll = 0.0;
    Ki_roll = 0.0;

    Kp_pitch = 1000;
    Kd_pitch = 0.0;
    Ki_pitch = 0.0;

    Kp_yaw = 0.0;
    Kd_yaw = 0.0;
    Ki_yaw = 0.0;

    Kp_alt = 1.0;
    Kd_alt = 0.75; 
    Ki_alt = 0.0;

    Kp_roll_rate = 0.0001;
    Kd_roll_rate = 0.00002;
    Ki_roll_rate = 0.0000;

    Kp_pitch_rate = 0.0001;
    Kd_pitch_rate = 0.00002;
    Ki_pitch_rate = 0.00;

    Kp_yaw_rate = 0.0005;
    Kd_yaw_rate = 0.0;
    Ki_yaw_rate = 0.0005;
}

void UAV_PID::init() {
  pidRoll = new PID(&state_roll, &out_roll, &set_roll, Kp_roll, Ki_roll, Kd_roll, DIRECT);
  pidPitch = new PID(&state_pitch, &out_pitch, &set_pitch, Kp_pitch, Ki_pitch, Kd_pitch, DIRECT);
  pidYaw = new PID(&state_yaw, &out_yaw, &set_yaw_err, Kp_yaw, Ki_yaw, Kd_yaw, DIRECT);
  pidAlt = new PID(&state_alt, &out_alt, &set_alt, Kp_alt, Ki_alt, Kd_alt, DIRECT);

  pidRollRate = new PID(&state_roll_rate, &out_roll_rate, &set_roll_rate, Kp_roll_rate, Ki_roll_rate, Kd_roll_rate, DIRECT);
  pidPitchRate = new PID(&state_pitch_rate, &out_pitch_rate, &set_pitch_rate, Kp_pitch_rate, Ki_pitch_rate, Kd_pitch_rate, DIRECT);
  pidYawRate = new PID(&state_yaw_rate, &out_yaw_rate, &set_yaw_rate, Kp_yaw_rate, Ki_yaw_rate, Kd_yaw_rate, DIRECT);

  pidRoll->SetMode(AUTOMATIC);
  pidPitch->SetMode(AUTOMATIC);
  pidYaw->SetMode(AUTOMATIC);
  pidAlt->SetMode(AUTOMATIC);

  pidRollRate->SetMode(AUTOMATIC);
  pidPitchRate->SetMode(AUTOMATIC);
  pidYawRate->SetMode(AUTOMATIC);

  pidRoll->SetOutputLimits(-1000, 1000);
  pidPitch->SetOutputLimits(-1000, 1000);
  pidYaw->SetOutputLimits(-100, 100);
  pidAlt->SetOutputLimits(-0.3, 0.3);

  pidRollRate->SetOutputLimits(-0.6, 0.6);
  pidPitchRate->SetOutputLimits(-0.6, 0.6);
  pidYawRate->SetOutputLimits(-0.6, 0.6);

  for (int i = 0; i < 4; i++) {
    pinMode(motor_pin[i], OUTPUT);
    analogWrite(motor_pin[i], 0);
  }
}

void UAV_PID::adjustAngleDifference(float new_angle) {
    float difference = set_yaw_abs - new_angle;

    while (difference < -M_PI) difference += M_PI;
    while (difference > M_PI) difference -= M_PI;

    state_yaw = difference;
}

void UAV_PID::compute(void) {
  pidRoll->Compute();
  pidPitch->Compute();
  pidYaw->Compute();
  pidAlt->Compute();

  updateSetpointsRate(out_roll,out_pitch,0);
  pidRollRate->Compute();
  pidPitchRate->Compute();
  pidYawRate->Compute();

  motor_speeds[2] = base_speed + out_alt + out_pitch_rate + out_roll_rate + out_yaw_rate;
  motor_speeds[3] = base_speed + out_alt - out_pitch_rate + out_roll_rate - out_yaw_rate;
  motor_speeds[0] = base_speed + out_alt - out_pitch_rate - out_roll_rate + out_yaw_rate;
  motor_speeds[1] = base_speed + out_alt + out_pitch_rate - out_roll_rate - out_yaw_rate;

  for(uint8_t i = 0; i < 4; i++) {
    motor_speeds[i] = constrain(motor_speeds[i]*255, 0.0, 255.0);
  }
  
}

void UAV_PID::updateSetpoints(double roll, double pitch, double yaw, double alt) {
  set_roll = roll;
  set_pitch = pitch;
  set_yaw_abs = yaw;
  set_yaw_err = 0;
  set_alt = alt;
}

void UAV_PID::updateState(double roll, double pitch, double yaw, double alt) {
  state_roll = roll;
  state_pitch = pitch;
  adjustAngleDifference(yaw);
  // state_alt = getAltitude(alt, roll, pitch);
  state_alt = alt;
}

void UAV_PID::updateSetpointsRate(double roll_rate, double pitch_rate, double yaw_rate) {
  set_roll_rate = roll_rate;
  set_pitch_rate = pitch_rate;
  set_yaw_rate = yaw_rate;
}

void UAV_PID::updateStateRate(double roll_rate, double pitch_rate, double yaw_rate) {
  state_roll_rate = roll_rate;
  state_pitch_rate = pitch_rate;
  state_yaw_rate = -yaw_rate;
}

void UAV_PID::setMotors(void) {
  for (int i = 0; i < 4; i++) {
    analogWrite(motor_pin[i], motor_speeds[i]);
  }
}

void UAV_PID::getMotors(double* m0, double* m1, double* m2, double* m3) {
  // *m0 = motor_speeds[0] * 4.2 * 14000 / 255;
  // *m1 = motor_speeds[1] * 4.2 * 14000 / 255;
  // *m2 = motor_speeds[2] * 4.2 * 14000 / 255;
  // *m3 = motor_speeds[3] * 4.2 * 14000 / 255;
  *m0 = motor_speeds[0] * 30000 / 255;
  *m1 = motor_speeds[1] * 30000 / 255;
  *m2 = motor_speeds[2] * 30000 / 255;
  *m3 = motor_speeds[3] * 30000 / 255;
}

void UAV_PID::printMotors(void) {
  Serial.print("R:\t");
  Serial.print(state_roll);
  Serial.print("\tP:\t");
  Serial.print(state_pitch);
  Serial.print("\tY:\t");
  Serial.println(state_yaw);
  Serial.print("PID_S: \tB: ");
  Serial.print(base_speed);
  Serial.print("\tR:\t");
  Serial.print(out_roll);
  Serial.print("\tP:\t");
  Serial.print(out_pitch);
  Serial.print("\tY:\t");
  Serial.print(out_yaw);
  Serial.print("\t:A\t");
  Serial.println(out_alt);
  Serial.print("PID_R: \tB: ");
  Serial.print(base_speed);
  Serial.print("\tR:\t");
  Serial.print(out_roll_rate);
  Serial.print("\tP:\t");
  Serial.print(out_pitch_rate);
  Serial.print("\tY:\t");
  Serial.print(out_yaw_rate);
  Serial.print("\t:A\t");
  Serial.println(out_alt);
  base_speed + out_alt + out_pitch + out_roll + out_yaw;
  Serial.print("Front: \t");
  Serial.print(motor_speeds[3] / 255);
  Serial.print("\t");
  Serial.println(motor_speeds[0] / 255);
  Serial.print("Back: \t");
  Serial.print(motor_speeds[2] / 255);
  Serial.print("\t");
  Serial.println(motor_speeds[1] / 255);
}

void UAV_PID::stopMotors(void) {
  for (int i = 0; i < 4; i++) {
    analogWrite(motor_pin[i], 0);
  }
}