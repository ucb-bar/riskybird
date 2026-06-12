// This is a simple Arduino program that sends the message "Hello, World!" over the serial port.

// The setup() function is called once when the Arduino starts up.
void setup() {
  // Initialize the serial communication at a baud rate of 115200 bits per second.
  // The baud rate is the speed at which data is sent or received. 115200 is a common speed and means 115200 bits per second.
  Serial.begin(115200);

  // Wait for the serial port to initialize. This delay is especially important for boards with a separate USB-to-Serial chip.
  delay(1000);  // This delay lasts for 1000 milliseconds, or 1 second.

  // Send the message "Hello, World!" over the serial port.
  Serial.println("Hello, World!");

  // The setup() function ends here, and the loop() function will start next.
}

// The loop() function is called repeatedly and is used to perform actions over and over again.
void loop() {
  // In this example, we won't send any more messages in the loop. 
  // So, the loop() function will stay empty.

  // However, if you wanted to repeatedly send messages, you could put the Serial.println() function here!
}