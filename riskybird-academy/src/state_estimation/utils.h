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

#endif