#ifndef __utils_h__
#define __utils_h__

#include <math.h>
#include "FastIMU.h"

#define SERIAL_BUFFER_LEN 256

struct Quaternion_t {
  double w, x, y, z;
};

struct Euler_t {
  double roll, pitch, yaw;
};

Euler_t quaternionToEuler(Quaternion_t q);
void parseSerialObservation(double* z, double* qw, double* qx, double* qy, double* qz, double* rollrate, double* pitchrate, double* yawrate);
void saveCalibrationData(calData& data);
bool loadCalibrationData(calData& data);

#endif