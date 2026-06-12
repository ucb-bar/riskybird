## **Real-World Flight Test with the RiskyBird**

After your intensive sessions with simulations and understanding the nuances of state estimation, it's time to see how the RiskyBird behaves in the real world!

### **1 Pre-flight Preparations**

Before initiating the flight, ensure the following:

1. The area is clear of obstacles.
2. All bystanders are at a safe distance.
3. The battery is appropriately connected by the instructor.
4. The RiskyBird's propellers are in good condition and securely attached.
5. The `#define REAL` flag in `src/riskybird_control/riskybird_control.ino` is uncommented.

**Safety First!** Always remember that safety is paramount. Drones, even smaller ones like the RiskyBird, can cause harm if not handled correctly.

### **2 Take Off!**

With the instructor's supervision, initiate the flight. The drone will fly for a short duration, about 1-2 seconds. Observe its behavior. 

🤔 **Discussion Point**: How does the RiskyBird's flight compare to your simulation results? Are there any noticeable differences in stability, responsiveness, or any other flight characteristics? What could be the reasons for any discrepancies between simulation and real-world behavior?

### **3 Deep Dive into PID Computations**

Navigate to `src/riskybird_control/uav_pid.cpp` and find the `UAV_PID::compute` method. This method is the heart of the PID controller for the RiskyBird.

The motor speeds are set based on the PID calculations for roll, pitch, yaw, and altitude. Given the drone's configuration, the calculated values determine the required thrust and torque to maintain or achieve the desired orientation and height.

🚀 **Activity**: Review the method and note how the motor speeds are assigned. 

🤔 **Discussion Point**: Drone dynamics are intricate. A quadcopter has four motors, and by varying their speeds, it can achieve different maneuvers. For instance:
- To move up or down, all motors adjust their speed uniformly.
- To pitch forward or backward, the front and rear motors vary their speeds relative to each other.
- To roll left or right, the left and right motors adjust their speeds.
- To yaw or rotate about its center, diagonal pairs of motors change speeds in opposing directions.

Considering the above, does the motor speed assignment in `UAV_PID::compute` make sense? What could be the implications if motor speed assignments were mixed up, say motor 1's speed being assigned to motor 3?

