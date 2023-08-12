## Pre-Lab Instructions: Setting up the ESP32-C3 with the Arduino IDE

### **Objective**:
By the end of this pre-lab, you will have successfully set up the ESP32-C3 board with the Arduino IDE and be ready to upload sketches to it.

### **Prerequisites**:
- **Arduino IDE** installed on your computer. If you haven't, download it from [Arduino's official website](https://www.arduino.cc/en/Main/Software).
- An **ESP32-C3** board and a suitable USB cable.

### **Instructions**:

1. **Open Arduino IDE**
    - Launch the Arduino IDE on your computer.

2. **Install the ESP32 Board Package**
    - Go to `Tools > Board > Boards Manager`.
    - In the search bar, type "esp32".
    - Find "esp32 by Espressif Systems" in the list and install it.
  
3. **Install the Required Libraries**
    - Go to `Tools > Manage Libraries...`.
    - Install the following Libraries:
      - Adafruit Neopixel (By Adafruit)
      - FastIMU (By LiquidGCS)
      - JPEGDEC (By Larry Bank)
      - PID (By Brett Beauregard)
      - TensorFlowLite_ESP32 (By TensorFlow)
      - VL53L1X (By Pololu)

4. **Select Your Board**
    - Go to `Tools > Board`.
    - Scroll down the list and select `ESP32-C3 Dev Module`.

5. **Select the COM Port**
    - With the ESP32-C3 board connected to your computer via USB, go to `Tools > Port`.
    - Select the COM port that the ESP32-C3 is connected to. It's often labeled as "Silicon Labs" or "esp32" in the menu.

6. **Test the Setup**
    - Open the "Blink" example by going to `File > Examples > 01.Basics > Blink`.
    - Click the "Upload" button (the right-facing arrow).
    - After the upload is complete, you should see the on-board LED blinking. If it does, congratulations! Your setup is correct.

### **Conclusion**:
You've successfully set up the Arduino IDE to work with the ESP32-C3. You're now ready to start the main lab exercise and upload your sketches to the ESP32-C3.

## Lab Exercise 1: Sending "Hello World" Over the Serial Port

### **Objective**:
By the end of this lab, you will have learned how to send the "Hello World" message to a computer from your ESP32-C3 board using the Serial port.

### **Prerequisites**:
- Completed the pre-lab setup for the ESP32-C3 with the Arduino IDE.
- A computer with a USB port.

### **Files**:
- Source file: `src/hello_world/hello_world.ino`

### **Instructions**:

1. **Open the Source File**
    - Navigate to the `src/hello_world` directory.
    - Open the `hello_world.ino` file in the Arduino IDE.

2. **Examine the Code**
    - Before uploading any sketch, it's a good habit to review it. This code initializes the serial communication and sends "Hello World" over the Serial port.
    ```cpp
    void setup() {
      Serial.begin(115200); // Initialize serial communication at a baud rate of 115200
      delay(1000);          // Wait for a second
      Serial.println("Hello World!"); // Send the message
    }

    void loop() {
      // Nothing here for this exercise
    }
    ```

3. **Upload the Sketch**
    - Ensure your ESP32-C3 board is connected to your computer via USB.
    - Click the "Upload" button (the right-facing arrow) in the Arduino IDE.
    - Wait for the message "Done uploading" to appear at the bottom.

4. **Open the Serial Monitor**
    - After uploading, go to `Tools > Serial Monitor` or press `Ctrl + Shift + M` (on Windows/Linux) or `Cmd + Shift + M` (on macOS).
    - Ensure the baud rate in the bottom right corner of the Serial Monitor is set to `115200`.

5. **View the Message**
    - You should see "Hello World!" displayed in the Serial Monitor.

### **Discussion Points**:

1. What is a baud rate?
2. What happens if you change the baud rate in the Serial Monitor but not in the code (or vice versa)?
3. How can you modify the code to send a different message or send messages repeatedly?

### **Conclusion**:
You've successfully sent a message from the ESP32-C3 to your computer using the Serial port. This basic communication is foundational for many IoT (Internet of Things) projects and other applications involving the ESP32-C3.


## **Lab 2: Controlling the RGB LED on ESP32-C3**

### **Objective**:
Get familiarized with the ESP32-C3's built-in RGB LED, learn to control its colors, and understand basic functions of the Arduino platform.

### **Prerequisites**:

- ESP32-C3 development board.
- USB cable.
- Computer with Arduino IDE installed.

### **Background**:

ESP32-C3 comes with a built-in RGB LED, which can display a variety of colors by mixing red, green, and blue light. In this lab, we'll be using the Adafruit_NeoPixel library to control this LED and make it change colors.

### **Instructions**:

1. **Setup**:
    - Connect the ESP32-C3 board to the computer using a USB cable.
    - Open the Arduino IDE on your computer.
    - Under `Tools > Board`, select `ESP32-C3 Dev Module`.
    - Make sure the correct COM port is selected under `Tools > Port`.

2. **Loading the Code**:
    - Navigate to `src/rgb_led/rgb_led.ino` in the Arduino IDE.
    - Open the file.

3. **Understanding the Code**:
    - The `#include` directive at the top allows us to use the Adafruit_NeoPixel library.
    - Constants like `PIN`, `NUMPIXELS`, and `BRIGHTNESS` are defined for easy reference.
    - The `pixels` object is an instance of `Adafruit_NeoPixel` and represents the built-in RGB LED.
    - The `setColor` function sets the color of the RGB LED based on the RGB values passed to it.
    - In the `loop` function, we will set the RGB LED to various colors with a 1-second delay between each color change.

4. **Modifying the Code**:
    - **TODO 1**: Define the brightness of the NeoPixel by setting a value for `BRIGHTNESS` between 0 (off) and 255 (full brightness).
    - **TODO 2**: Set the RGB LED's color to red using the `setColor` function.
    - **TODO 3**: Similarly, set the LED's color to green.
    - **TODO 4**: Finally, set the LED's color to blue.

5. **Uploading and Running**:
    - Once you've made the changes, click on the `Upload` button (the right arrow icon) in the Arduino IDE. This will compile and upload your code to the ESP32-C3.
    - After uploading, you should see the RGB LED on the ESP32-C3 change colors: red, green, and blue, each lasting for 1 second.

### **Questions**:

1. What happens if you set the `BRIGHTNESS` value to 0? What about 255?
2. Can you make the RGB LED display a yellow color? (Hint: Yellow is made by mixing red and green.)
3. How would you modify the code to reduce the delay between color changes?

### **Challenge**:

Can you modify the code to make the RGB LED fade smoothly between colors?

## **Lab3: Permanent Data Storage with the ESP32's Preferences Library**

### **Objective**:
Learn how to use the Preferences library to store and retrieve data that persists across power cycles on the ESP32.

### **Prerequisites**:

- ESP32-C3 development board.
- USB cable.
- Computer with Arduino IDE installed.

### **Background**:

ESP32's non-volatile storage (NVS) allows data to be stored even when the board is powered off. This feature can be useful for settings, high scores in games, or counters that shouldn't reset every time the device restarts. However, the NVS has a finite number of write cycles, so it's crucial to use it judiciously.

### **Instructions**:

1. **Setup**:
    - Connect the ESP32-C3 board to the computer using a USB cable.
    - Open the Arduino IDE.
    - Under `Tools > Board`, select `ESP32-C3 Dev Module`.
    - Ensure the correct COM port is selected under `Tools > Port`.

2. **Loading the Code**:
    - Open `src/counter_nvs/counter_nvs.ino` in Arduino IDE.

3. **Modifying the Code**:
    - **TODO 1**: Use the `preferences.getInt` function to retrieve the `count` value. If it doesn't exist, set it to 0.
    - **TODO 2**: Print the `count` value to the Serial Monitor using `Serial.println()`.
    - **TODO 3**: Increment the `count` value.
    - **TODO 4**: Store the updated `count` value to NVS using `preferences.putInt`.

4. **Uploading and Testing**:
    - Click the `Upload` button in the Arduino IDE.
    - Once uploaded, open the Serial Monitor.
    - You should see a count value printed. Restart or reset the ESP32 and observe the count value again. It should have incremented from the previous value, demonstrating that the value persists across restarts.

### **Questions**:

1. What are the benefits of using non-volatile storage like the NVS in ESP32?
2. What will happen if you exceed the maximum write cycles of NVS?
3. Why is it crucial to close the Preferences after using it (`preferences.end()`)?

### **Pitfalls & Considerations**:

- **Write Cycles**: NVS memory has a limited number of write cycles (typically around 100,000 times for each block). Continuously writing to NVS can wear it out.
  
- **Space Limitation**: ESP32 has limited NVS space. Always check if there's enough space before writing large data.
  
- **Multiple Projects**: If students use the same ESP32 board for different projects, they might overwrite data from previous projects. Always use unique namespace names and key names to avoid data collision.

### **Challenge**:

1. Can you modify the code to store more than one value, for example, `count1` and `count2`?
2. What are some techniques you could use to reduce the impact of frequenct writes to the NVS? (Hint: consider data that are written often but change rarely)

