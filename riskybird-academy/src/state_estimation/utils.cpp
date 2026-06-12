#include "utils.h"
#include <Preferences.h>

Preferences preferences;
char buf[SERIAL_BUFFER_LEN];

Euler_t quaternionToEuler(Quaternion_t q) {
  Euler_t e;

  // Roll (x-axis rotation)
  float sinr_cosp = 2 * (q.w * q.x + q.y * q.z);
  float cosr_cosp = 1 - 2 * (q.x * q.x + q.y * q.y);
  e.roll = atan2(sinr_cosp, cosr_cosp);

  // Pitch (y-axis rotation)
  float sinp = 2 * (q.w * q.y - q.z * q.x);
  if (abs(sinp) >= 1)
    e.pitch = copysign(M_PI / 2, sinp); // Use 90 degrees if out of range
  else
    e.pitch = asin(sinp);

  // Yaw (z-axis rotation)
  float siny_cosp = 2 * (q.w * q.z + q.x * q.y);
  float cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z);
  e.yaw = atan2(siny_cosp, cosy_cosp);

  return e;
}

void parseSerialObservation(double* z, double* qw, double* qx, double* qy, double* qz, double* rollrate, double* pitchrate, double* yawrate) {
    char* idx;
    int newlinesCount = 0;  // Counter for newlines
    int i = 0;  // Index for inputString

    // Wait until three newlines are received or buffer is full
    while (newlinesCount < 3 && i < SERIAL_BUFFER_LEN - 1) {
        if (Serial.available()) {
            char inChar = (char)Serial.read();
            if (inChar == '\n') {
                newlinesCount++;
            }
            buf[i++] = inChar;
        }
    }
    buf[i] = '\0';
    idx = strstr(buf, "x:");
    double x,y;
    if (idx != NULL) {
        sscanf(idx, "x: [%lf %lf %lf]", &x, &y, z);
    }

    idx = strstr(buf, "theta:");
    if (idx != NULL) {
        sscanf(idx, "theta: [%lf %lf %lf %lf]", qx, qy, qz, qw);
    }

    idx = strstr(buf, "omega:");
    if (idx != NULL) {
        sscanf(idx, "omega: [%lf %lf %lf]", rollrate, pitchrate, yawrate);
    }
}

void saveCalibrationData(calData& data) {
  Serial.println("Saving calibration preferences");
  Serial.print("Valid: ");
  Serial.println(data.valid);
	preferences.begin("riskybird", false);
	preferences.putBool("valid", data.valid);
	preferences.putBytes("accelBias", data.accelBias, sizeof(data.accelBias));
	preferences.putBytes("gyroBias", data.gyroBias, sizeof(data.gyroBias));
	preferences.putBytes("magBias", data.magBias, sizeof(data.magBias));
	preferences.putBytes("magScale", data.magScale, sizeof(data.magScale));
	preferences.end();
}

bool loadCalibrationData(calData& data) {
  Serial.println("Loading calibration preferences");
	preferences.begin("riskybird", true);
	data.valid = preferences.getBool("valid", false); // second parameter is default value
	if (!data.valid) {
		// data is not valid, don't load
    Serial.println("Invalid calibration.");
		preferences.end();
		return false;
	}
	preferences.getBytes("accelBias", data.accelBias, sizeof(data.accelBias));
	preferences.getBytes("gyroBias", data.gyroBias, sizeof(data.gyroBias));
	preferences.getBytes("magBias", data.magBias, sizeof(data.magBias));
	preferences.getBytes("magScale", data.magScale, sizeof(data.magScale));
	preferences.end();
	return true;
}