#include "tof.h"

ToF::ToF() {
  pca_port = 0;
}

void ToF::PCA9534_pinMode(uint8_t pin, uint8_t mode) {
  switch (mode) {
    case OUTPUT:
      // Clear the pin on the configuration register for output
      pca_port &= ~(1 << pin);
      break;
    case INPUT:
    default:
      // Set the pin on the configuration register for input
      pca_port |= (1 << pin);
      break;
  }

  // Write the configuration of the individual pins as inputs or outputs
  Wire.beginTransmission(PCA9534_I2C_ADDRESS);
  Wire.write(PCA9534_CONF_REGISTER);
  Wire.write(pca_port);
  Wire.endTransmission();

  if (mode == INPUT) {
    // Write the input polarity configuration of the individual pins
    Wire.beginTransmission(PCA9534_I2C_ADDRESS);
    Wire.write(PCA9534_INV_REGISTER);
    Wire.write(0x00);
    Wire.endTransmission();
  }
}

void ToF::PCA9534_digitalWrite(uint8_t pin, uint8_t value) {
  switch (value) {
    case HIGH:
      // Set the pin HIGH on the output register
      pca_port |= (1 << pin);
      break;
    case LOW:
    default:
      // Set the pin LOW on the output register
      pca_port &= ~(1 << pin);
      break;
  }
  // Write the status of the pins on the output register
  Wire.beginTransmission(PCA9534_I2C_ADDRESS);
  Wire.write(PCA9534_OP_REGISTER);
  Wire.write(pca_port);
  Wire.endTransmission();
}

uint8_t ToF::PCA9534_digitalRead(uint8_t pin) {
  uint8_t buff = 0;

  // We need to write to the input register first to get the status of the pins.
  // As per the specs: "a write transmission is sent with the command byte to
  // let the I2C device know that the Input Port register will be accessed
  // next."
  Wire.beginTransmission(PCA9534_I2C_ADDRESS);
  Wire.write(PCA9534_IP_REGISTER);
  Wire.write(pca_port);
  Wire.endTransmission();

  // We only need to read 1 byte of data to get the pins
  Wire.requestFrom(PCA9534_I2C_ADDRESS, 1);
  while (Wire.available()) {
    buff = Wire.read();
  }
  return (buff & (1 << pin)) ? HIGH : LOW;
}

void ToF::check_addrs() {
  int nDevices = 0;
  byte error, address;
  for(address = 1; address < 127; address++ ) 
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address<16) 
        Serial.print("0");
      Serial.print(address,HEX);
      Serial.println("  !");

      nDevices++;
    }
    else if (error==4) 
    {
      Serial.print("Unknow error at address 0x");
      if (address<16) 
        Serial.print("0");
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");
}

void ToF::init_tof() {
  for (int i = 0; i < NUM_TOF; i++) {
    PCA9534_pinMode(tof_pin[i], OUTPUT);
    PCA9534_digitalWrite(tof_pin[i], LOW);
  }
  for (int i = 0; i < NUM_TOF; i++) {
    PCA9534_digitalWrite(tof_pin[i], HIGH);
    delay(50);
    if (!tof_sensors[i].init()){
      Serial.println("Failed to detect and initialize ToF sensor!");
      while (1);
    }
    tof_sensors[i].setAddress(0x2A + i);
    tof_sensors[i].setDistanceMode(VL53L1X::Long);
    tof_sensors[i].setMeasurementTimingBudget(50000);

  // Start continuous readings at a rate of one measurement every 50 ms (the
  // inter-measurement period). This period should be at least as long as the
  // timing budget.
    tof_sensors[i].startContinuous(50);
  }
}