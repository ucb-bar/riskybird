#ifndef __uav_pid_h__
#define __uav_pid_h__

#include <stdint.h>
#include <PID_v1.h>

class UAV_PID {
  private:
    double Kp_roll, Kd_roll, Ki_roll;
    double Kp_pitch, Kd_pitch, Ki_pitch;
    double Kp_yaw, Kd_yaw, Ki_yaw;
    double Kp_alt, Kd_alt, Ki_alt;

    double set_roll, set_pitch, set_alt;
    double set_yaw_abs, set_yaw_err;
    double state_roll, state_pitch, state_yaw, state_alt;
    double out_roll, out_pitch, out_yaw, out_alt;

    double Kp_roll_rate, Kd_roll_rate, Ki_roll_rate;
    double Kp_pitch_rate, Kd_pitch_rate, Ki_pitch_rate;
    double Kp_yaw_rate, Kd_yaw_rate, Ki_yaw_rate;
    double Kp_alt_rate, Kd_alt_rate, Ki_alt_rate;

    double set_roll_rate, set_pitch_rate, set_yaw_rate;
    double state_roll_rate, state_pitch_rate, state_yaw_rate;
    double out_roll_rate, out_pitch_rate, out_yaw_rate;

    double base_speed;

    double motor_speeds[4];
    uint8_t motor_pin[5] = {2, 3, 4, 5};

    PID * pidRoll;
    PID * pidPitch;
    PID * pidYaw;
    PID * pidAlt;

    PID * pidRollRate;
    PID * pidPitchRate;
    PID * pidYawRate;


  public:
    UAV_PID(void);
    void init(void);
    void compute(void);
    void adjustAngleDifference(float new_angle);
    void updateSetpoints(double roll, double pitch, double yaw, double alt);
    void updateState(double roll, double pitch, double yaw, double alt);
    void updateSetpointsRate(double roll_rate, double pitch_rate, double yaw_rate);
    void updateStateRate(double roll_rate, double pitch_rate, double yaw_rate);
    void setMotors(void);
    void getMotors(double* m0, double* m1, double* m2, double* m3);
    void printMotors(void);
    void stopMotors(void);

};

#endif