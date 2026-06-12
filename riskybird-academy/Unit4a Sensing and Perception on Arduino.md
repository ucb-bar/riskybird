## Person detection example

This example shows how you can use Tensorflow Lite to run a pre-trained neural network to recognize people in images on a microcontroller.


### Download the Pre-Trained Model

Download the trained model file (C source file [person_detect_model_data.cc](src/person_detection_camera/person_detect_model_data.cpp)) from src. To understand how we build the model and TensorFlow Lite does this, you can look at scripts/person_detection


### Port it to Arduino:

To run person detection 



# **RiskyBird Drone Activities Lab**

Welcome to the RiskyBird Drone Activities Lab! In this session, you will be working with the RiskyBird drone, powered by an ESP32-C3 processor. Get ready to dive into hands-on activities involving cameras, neural networks, and drone state estimation!

## **1. Serial TTL Camera Activity**

**Objective**: Familiarize yourself with the Serial TTL Camera on the RiskyBird.

### **Background**
Serial TTL (Transistor-Transistor Logic) cameras capture images and send them over a serial connection. This type of connection simplifies wiring and integration with devices like the ESP32. However, serial connections, especially at lower baud rates, can introduce latency compared to parallel data transfers.

### **1.1 Setting Up Camera Test**

- Open the Arduino project at `src/camera_test/camera_test.ino`.
- Upload the code to your RiskyBird.
- Modify the `PORT` variable in `scripts/person_detection/display_image.py` to match your Arduino IDE's port.

### **1.2 Displaying Camera Feed**

With the Arduino connected and running, execute:

```bash
python scripts/person_detection/display_image.py
```
This will allow you to view the camera data playback.

### **1.3 Decoding Images**

The camera provides JPEG images, transmitted over the serial port in compressed form. For future activities, decoding the image on the ESP32 is vital. 

🚀 **Activity**: Uncomment the `#define DECODE` in `camera_test.ino`. Upload the code again and try the `display_image.py` script. You should now see a decoded grayscale image.

## **2. Person Detection Activity**

**Objective**: Run a person detection algorithm using a MobileNet DNN.

### **Background: Model Architecture**

[MobileNets](https://arxiv.org/abs/1704.04861) are a family of efficient Convolutional Neural Networks for Mobile Vision, designed to provide good accuracy for as few weight parameters and arithmetic operations as possible.

For the time manner, we have pre-built the model. What you need to do is to load the pre-trained TensorFlow model, run some example images through it, and get the expected outputs. 

### **2.1 Setting Up Person Detection**

- Open the Arduino project at `src/person_detection/person_detection.ino`.
- Upload the code to your RiskyBird.

When the code is executed, the LED will:

- Light up **green** when a person is detected.
- Light up **red** when no person is detected.
- Light up **yellow** when uncertain.

#### **Task Complexity: Cartpole vs. Image Classification**

- **Cartpole Control**: This task primarily requires processing input from sensors like the gyroscope and accelerometer and producing a control signal. The input data size is relatively small, and the computations, though real-time, are not as computationally intense.

- **Image Classification**: Here, the input is an image, which even when reduced in resolution, consists of a large number of pixels. Each pixel is an input feature. Deep learning models like MobileNet can have millions of parameters, so the computations involved are significantly more complex.

🤔 **Discussion Point**: Consider the number of inputs (pixels in an image vs. sensor readings for the cartpole). Why might processing an image require more computational resources than controlling the cartpole?

#### **Latency Impacts**

The camera's slow serial connection introduces a noticeable latency in capturing and transmitting the image. This latency, combined with the computational time for image classification, can lead to delays in response. In contrast, the IMU provides fast, real-time data, allowing for swift reactions.

🤔 **Discussion Point**: In real-world applications like surveillance drones or rescue drones, why is it crucial to minimize this latency? How might it affect the drone's performance or mission success?

### **2.2 Analyzing the DNN Model Data**

Open `person_detect_model_data.cpp`.

🤔 **Discussion Point**: Compare the size of the `tflite` data array to a cartpole DNN used earlier. Why might there be differences?

## **3. Drone State Estimation Activity**

**Objective**: Understand how an IMU determines a drone's orientation and learn to read data from a time of flight sensor.

### **3.1 Setting Up State Estimation**

- Open the Arduino project at `src/state_estimation/state_estimation.ino`.
- Upload the code to your RiskyBird.

### **3.2 Calibration and Viewing Data**

While connected, watch the serial monitor:

1. Move the RiskyBird in a figure-eight pattern for magnetometer calibration.
2. Place the drone level on a table for accelerometer calibration.

After calibration, the estimated roll/pitch/yaw, angular rate, and time of flight measurements are displayed.

🤔 **Discussion Point**: How accurate do you find the results? Consider how the gyroscope gives change in orientation, and the accelerometer and magnetometer provide absolute orientation. How do they achieve this? Think about Earth's gravitational and magnetic fields.

### **3.3 Orientation and Positioning**

**Objective**: Understand the significance of gyroscopes in estimating drone orientation and the challenges of using it solely for positional tracking.

#### **The Gyroscope**

The gyroscope in the IMU measures the angular rate of change, which can be used to determine the drone's orientation. However, by itself, it's only capable of informing about how fast the drone is rotating, not the drone's absolute orientation in space.

🚀 **Activity**: Rotate the RiskyBird manually and observe how the gyroscope readings change. Try slow and fast rotations. Note the readings in each case.

#### **The Challenge with Integration**

If we solely rely on the gyroscope to determine the drone's position, we would be integrating these rate of change values over time. However, this method, called "dead reckoning," accumulates errors over time. Small inaccuracies in the gyroscope readings can lead to significant errors when integrated over extended periods, causing the estimated position to drift from the real position.

🤔 **Discussion Point**: Can you think of a scenario where even a small drift might be problematic for a drone? Consider a drone delivering a package or one flying indoors.

#### **Madgwick Filter: A Solution**

The Madgwick filter is a sensor fusion algorithm. It combines the absolute orientation data from the accelerometer and magnetometer with the gyroscope's rate of change. This fusion compensates for the individual limitations of each sensor and provides a more accurate estimation of the drone's orientation.

🤔 **Discussion Point**: Why might combining data from different sensors lead to a more accurate estimation? Are there any other sensors that would be useful here? Consider cases where you need the drone's absolute position in addition to absolute orientation.

### **3.4 Optional: Uncalibrated Data Analysis**

**Objective**: Recognize the importance of sensor calibration in ensuring accurate readings and estimations.

#### **What is Calibration?**

Calibration is the process of aligning sensor readings with known values. For example, when the RiskyBird is stationary, the accelerometer should read a specific value corresponding to the force of gravity, and the gyroscope should read zero (no rotation).

#### **Impact of Uncalibrated Data**

When data is uncalibrated, even the sensor's rest state might provide non-zero or inaccurate readings. Using uncalibrated data can significantly affect algorithms like the Madgwick filter, which rely on precise readings to provide accurate estimations.

🚀 **Activity**: With the `#define PERFORM_CALIBRATION` line commented out in `state_estimation.ino`, upload the code. Observe the output values while the drone is stationary. How do these readings compare to the calibrated version?

#### **Calibration in Real-world Scenarios**

In many real-world scenarios, drones and other devices recalibrate sensors frequently. Environmental factors like temperature, humidity, or even the device's battery level can influence sensor readings. Regular calibration ensures that the device operates within an acceptable margin of error.

🤔 **Discussion Point**: In what scenarios might regular recalibration be especially crucial? Consider a drone used for precision agriculture or a surveillance drone.


