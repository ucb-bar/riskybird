# **RiskyBird Drone PID Control Lab**

Welcome to the RiskyBird Drone PID Control Lab! Today, you'll dive into the world of drones, understand the concept of PID control, and experiment with a real-time simulator to see the effects of different PID settings.

## **Objectives:**

- Understand the basics of PID control.
- Experience a Hardware-in-Loop (HIL) simulation using RiskyBird.
- Analyze the effect of noise and PID parameters on drone stability.

## **1. Setting Up**

Let's get your environment ready to simulate the drone's flight.

### **1.1 Installing Dependencies**

In the RiskyBird repo, initialize and update submodules, then install the necessary Python packages:

```bash
git submodule update --init --recursive
cd sim/gym-pybullet-drones/
pip3 install --upgrade pip
pip3 install -e .
pip3 install pyserial
```

### **1.2 Uploading the Arduino Project**

Open the Arduino project located at `src/riskybird_control/riskybird_control.ino` and upload it to your Arduino board.

## **2. Running the Simulation**

Now, you'll run a simulator that visualizes the drone's flight and shows a graph of its state and motor RPMs over time.

### **2.1 Configure the Serial Port**

Open `sim/gym-pybullet-drones/gym_pybullet_drones/examples/pid_hil.py` and set the `PORT` variable (found below the import statements) to match the serial port used in your Arduino IDE.

### **2.2 Simulate the Drone's Hover**

Run the python file. Watch the drone try to maintain a hover.

🤔 **Discussion Point:** How does the drone perform?

### **2.3 Introducing Noise**

Modify the `pid_hil.py` file to set the `NOISE` variable (found below the includes) to `True`. This simulates noisy data sent to the ESP32-C3 controller.

Run the simulation again.

🤔 **Discussion Point:** How does noise impact the drone's behavior?

## **3. PID Tuning and Analysis**

You'll now dig into the code to understand how the PID controller works and experiment with different PID values.

### **3.1 Analyzing the PID Constructor**

Open `src/riskybird_control/uav_pid.cpp`. Here you'll find the PID constructor function:

```cpp
UAV_PID::UAV_PID(void) {
    // ... [code with PID values]
}
```

🤔 **Discussion Point:** What do you think will happen if all these values are set to 0?

### **3.2 Experiment: Zeroing the PID Values**

Change all PID values in the constructor to `0`, upload the updated code to the ESP32, and then run the python simulator.

🚀 **Activity:** What do you observe when all PID values were set to 0?

### **3.3 Tweaking the PID Values**

Proportional, Integral, and Derivative (PID) are the three constants used to fine-tune a control loop system.

- **Proportional Gain (`Kp`)**: Determines how aggressively the PID reacts to the current error. A high value might cause the system to oscillate.
- **Integral Gain (`Ki`)**: Combats the cumulative error. If the error has been present for an extended time, it will accumulate and the integral term will correct it.
- **Derivative Gain (`Kd`)**: Predicts system behavior. If the error is rapidly changing, the derivative term will counteract it.

🚀 **Activity:** Change different PID gains to see their effect on the drone's behavior.

## **4. Conclusion**

Congratulations! You've learned the basics of PID control and applied them to a real-world drone simulation. Understanding and tuning these parameters is crucial for a variety of robotic and automation tasks. Continue experimenting and happy flying!
