## Lab: Deploying a Reinforcement Learning Agent on ESP32-C3 with TensorFlow Lite

### Objective

In this lab, you will learn how to deploy a reinforcement learning (RL) agent onto an ESP32-C3 microcontroller. By using TensorFlow Lite for Microcontrollers, you will implement a policy to balance the pole in the classic `CartPole` problem.

### Background

`CartPole` is a classical problem in the reinforcement learning world. The goal is to balance a pole, hinged to a cart, by moving the cart left or right.

The ESP32-C3 microcontroller will be running the inference part, using a pre-trained model. The Python script provided will serve as the environment (the gym) and will communicate with the ESP32-C3 over serial to send observations and receive actions.

### Pre-requisites

- Familiarity with reinforcement learning concepts.
- Basic knowledge of Python.
- Previous lab on training the RL agent and generating the C++ file for the model.

### Instructions

#### 1. Environment Setup

1. Ensure you have the required hardware:
   - ESP32-C3 microcontroller.
   - USB Cable to connect ESP32-C3 to your computer.
  
2. Install necessary Python packages:
   ```bash
   pip install gym pyserial opencv-python
   ```

3. Ensure you have the Arduino IDE installed with support for ESP32-C3.

#### 2. Deploying the Model to ESP32-C3

1. Navigate to `scripts/RL_cartpole/PPO_cartpole/files/`.
2. Open the `actor_cartpole_model_data.cc` file. This file contains the trained model in a C++ array format.
3. Copy the entire contents of `actor_cartpole_model_data.cc`.
4. Now, navigate to `src/cartpole_tflite/`.
5. Open the `cartpole_tflite.ino` Arduino code.
6. Open the `model.cpp` file in a text editor or the Arduino IDE and replace its contents below `#include "model.h"` with the copied code.
7. Upload the code to the ESP32-C3.

#### 3. Running the Gym Environment

1. Connect the ESP32-C3 to your computer via the USB cable.
2. Navigate to `scripts/RL_cartpole/PPO_cartpole/`.
3. Open cartpole_arduino.py and set the PORT variable (found below the import statements) to match the serial port used in your Arduino IDE.
4. Run the `cartpole_arduino.py` script:
   ```bash
   python cartpole_arduino.py
   ```

5. You should see the `CartPole` environment window pop up, showing the cart and pole in action. The ESP32-C3 uses the TensorFlow Lite model to determine actions based on the observations it receives from the Python script.

#### 4. Observations and Conclusions

1. Observe how well the pole is balanced by the ESP32-C3.
2. Note the score at the end of each episode. How often can the agent successfully balance the pole for the full duration?
3. Reflect on the benefits and challenges of deploying machine learning models on microcontrollers.

### Additional Exercises

1. **Model Variation**: Go back to the previous lab and tweak the training parameters. Generate a new `actor_cartpole_model_data.cc` and deploy it. Does the agent's performance improve?
2. **Communication Delays**: Introduce artificial delays in the Python script's serial communication. How does this affect the agent's performance?

### Conclusion

Congratulations on successfully deploying a reinforcement learning agent on a microcontroller! This lab illustrated the power and potential of edge AI, where inference is run on-device rather than relying on cloud servers. Such deployments are crucial for real-time applications where low latency is essential.


