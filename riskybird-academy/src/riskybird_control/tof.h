#ifndef __tof_h__
#define __tof_h__
#include <Wire.h>
#include <VL53L1X.h>


#define PCA9534_ADDRESS 0x20  // Replace with your PCA9534's I2C address
#define PCA9534_OUTPUT_REGISTER 0x01


#define PCA9534_I2C_ADDRESS     0x20
#define PCA9534_IP_REGISTER     0x00
#define PCA9534_OP_REGISTER     0x01
#define PCA9534_INV_REGISTER    0x02
#define PCA9534_CONF_REGISTER   0x03

#define NUM_TOF   5
#define TOF_DOWN  0 //0
#define TOF_BACK  1 //1
#define TOF_RIGHT 2 //2
#define TOF_FRONT 3 //4
#define TOF_LEFT  4 //6



class ToF {
  private:
    uint8_t tof_pin[5] = {0, 1, 2, 4, 6};
    uint8_t pca_port = 0;

  public: 
    VL53L1X tof_sensors[5];

    ToF(void);
    void PCA9534_pinMode(uint8_t pin, uint8_t mode);
    void PCA9534_digitalWrite(uint8_t pin, uint8_t value) ;
    uint8_t PCA9534_digitalRead(uint8_t pin);
    void check_addrs(); 
    void init_tof();
};



#endif