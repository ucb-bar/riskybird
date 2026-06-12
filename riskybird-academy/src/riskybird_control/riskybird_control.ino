#include "FastIMU.h"
#include "Madgwick.h"
#include "tof.h"
#include "uav_pid.h"
#include "utils.h"

#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <WiFi.h>


#define IMU_ADDRESS 0x68    //Change to the address of the IMU
#define PERFORM_CALIBRATION //Comment to disable startup calibration
// #define REAL                //Comment to run PID loops in simulation

#define PIN 8               // Pin for RGB LEDs
#define NUMPIXELS 2         // Number of RGB LEDs
#define BRIGHTNESS 20       // Default brightness for RGB LEDs

// Create RGB LEDs object
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// Time of flight (depth) sensor
ToF tof;

// Inertial measurement unit 
MPU9250 IMU;          

//Calibration data
calData calib = { 0 };  

//Sensor data
AccelData IMUAccel;    
GyroData IMUGyro;
MagData IMUMag;

// State estimation filter
Madgwick filter;

// PID controller
UAV_PID pid;

// Timing and loop counting
uint32_t count;
unsigned long current_time;

// Set the color of an indexed RGB LED
void setLED(int i, int r, int g, int b) {
  pixels.setPixelColor(i, pixels.Color(r,g,b));
  pixels.show();
}

// Set the 2nd RGB LED to yellow
void statusActive() {
  setLED(1, BRIGHTNESS*0.8, BRIGHTNESS*0.6, 0);
}

// Set the 2nd RGB LED to red
void statusError() {
  setLED(1, BRIGHTNESS, 0, 0);
}

// Set the 2nd RGB LED to green
void statusReady() {
  setLED(1, 0, BRIGHTNESS, 0);
}

// Initialize IMU, with calibration
void init_imu() {
  int err = IMU.init(calib, IMU_ADDRESS);
  if (err != 0) {
    Serial.printf("Error initializing IMU: %d\n", err);
    statusError();
    while (true) {
      ;
    }
  }

#ifdef PERFORM_CALIBRATION
  // Check for pre-existing calibration data, if not, re-calibrate
  if(!loadCalibrationData(calib)) {
  // if(true) {
    Serial.println("FastIMU Calibrated Quaternion example");
    if (IMU.hasMagnetometer()) {
      delay(1000);
      // Set first RGB LED to purple until done
      setLED(0, BRIGHTNESS, 0 , BRIGHTNESS);
      Serial.println("Move IMU in figure 8 pattern until done.");
      delay(3000);
      IMU.calibrateMag(&calib);
      Serial.println("Magnetic calibration done!");
    }
    else {
      delay(1000);
    }
    Serial.println("Keep IMU level.");
    // Set first RGB LED to yellow until done
    setLED(0, BRIGHTNESS, BRIGHTNESS , 0 );
    delay(5000);
    
  
    IMU.calibrateAccelGyro(&calib);
    Serial.println("Calibration done!");
    saveCalibrationData(calib);
  }
  Serial.printf("Accel biases X/Y/Z: %.2f, %.2f, %.2f\n", 
                calib.accelBias[0], calib.accelBias[1], calib.accelBias[2]);

  Serial.printf("Gyro biases X/Y/Z: %.2f, %.2f, %.2f\n", 
                calib.gyroBias[0], calib.gyroBias[1], calib.gyroBias[2]);

  if (IMU.hasMagnetometer()) {
      Serial.printf("Mag biases X/Y/Z: %.2f, %.2f, %.2f\n", 
                    calib.magBias[0], calib.magBias[1], calib.magBias[2]);
      
      Serial.printf("Mag Scale X/Y/Z: %.2f, %.2f, %.2f\n", 
                    calib.magScale[0], calib.magScale[1], calib.magScale[2]);
  }

  #endif

  IMU.init(calib, IMU_ADDRESS);
  IMU.setIMUGeometry(3);
}

void setup() {
  pixels.begin(); // Start NeoPixel LEDs
  setLED(0, 0, 0, 0);
  statusActive();
  delay(200);

  Wire.begin(6,7);
  Wire.setClock(400000); //400khz clo
  Serial.begin(115200);
  
  while (!Serial) {
    ;
  }
  #ifdef REAL
    Serial.println("Booted up!");
  #endif
  setLED(0, 0, 0, 0 );
  #ifdef REAL
    init_imu();
    filter.begin(2.0f);
    tof.init_tof();
  #endif
  // Initialize the PID hiearchy
  pid.init();
  pid.updateSetpoints(0,0,0,0.5);
  statusReady();
}

void loop() {
  double z, qw, qx, qy, qz, rollrate, pitchrate, yawrate;
  double roll, pitch, yaw;

  // Run code when using real drone
  #ifdef REAL
    // Request the data from the IMUs, and store into variables
    IMU.update();
    IMU.getAccel(&IMUAccel);
    IMU.getGyro(&IMUGyro);
    IMU.getMag(&IMUMag);
   
    // Update state estimation using filter
    filter.update(IMUGyro.gyroX, IMUGyro.gyroY, IMUGyro.gyroZ, IMUAccel.accelX, IMUAccel.accelY, IMUAccel.accelZ, IMUMag.magX, IMUMag.magY, IMUMag.magZ);

    // Convert Quaternion to Euler Angle
    Quaternion_t q = {filter.getQuatW(), filter.getQuatX(), filter.getQuatY(), filter.getQuatZ()};
    Euler_t e = quaternionToEuler(q);

    // Extract angles into variables
    roll = e.roll;
    pitch = e.pitch;
    yaw = e.yaw;

    rollrate = IMUGyro.gyroX;
    pitchrate = IMUGyro.gyroY;
    yawrate = IMUGyro.gyroZ;

    float roll_in_degrees = degrees(e.roll);
    float pitch_in_degrees = degrees(e.pitch);
    float yaw_in_degrees = degrees(e.yaw);

    // Read data from ToF sensors
    uint16_t tof_data[5];
    for(uint8_t i = 0; i < NUM_TOF; i++) {
      tof_data[i] = tof.tof_sensors[i].read(false);
      if (tof.tof_sensors[i].timeoutOccurred()) { Serial.print(" TIMEOUT"); }
    }
    z = tof_data[0] / 1000.0;

  // Run code when using simulated drone
  #else
    // Read simulated observations from serial port
    parseSerialObservation(&z, &qw, &qx, &qy, &qz, &rollrate, &pitchrate, &yawrate);
    Quaternion_t q = {qw, qx, qy ,qz};
    Euler_t e = quaternionToEuler(q);

    roll = e.roll;
    pitch = e.pitch;
    yaw = e.yaw;
  #endif

  // Give new state estimate to PID controller
  pid.updateState(roll, pitch, yaw, z);
  pid.updateStateRate(rollrate, pitchrate, yawrate);

  // Compute new PID outputs
  pid.compute();

  // Only fly if using a real UAV
  #ifdef REAL
    // Run a short (1-2 second) flight for testing
    if (count < 1200 && count >= 1000) {
      pid.setMotors();
    } else {
      pid.stopMotors();
    }

    // Print occasional status updates
    if(count % 50 == 0) {
      unsigned long old_time = current_time;
      current_time = millis();
      Serial.printf("Roll: %.2f\tPitch: %.2f\tYaw: %.2f\tdRoll: %.2f\tdPitch: %.2f\tdYaw: %.2f\tToF:\t", 
                    roll_in_degrees, pitch_in_degrees, yaw_in_degrees, 
                    IMUGyro.gyroX, IMUGyro.gyroY, IMUGyro.gyroZ);

      for (uint8_t i = 0; i < NUM_TOF; i++) {
          Serial.printf("%d\t", tof_data[i]);
      }

      Serial.printf("(%ldms)\n", current_time - old_time);
      pid.printMotors();
      current_time = millis();
    }
  #else
    double m0, m1, m2, m3;
    pid.getMotors(&m0, &m1, &m2, &m3);
    Serial.printf("motors: [%e, %e, %e, %e]\n", m0, m1, m2, m3);
  #endif

  count++;
}
