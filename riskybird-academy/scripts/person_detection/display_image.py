import binascii
import numpy as np
import cv2
import serial

WIDTH = 160


# Configuration parameters
PORT = '/dev/ttyUSB0' # Adjust this to your Arduino's port
BAUD_RATE = 115200

# Open serial connection
ser = serial.Serial(PORT, BAUD_RATE)

def display_jpeg(hex_data):
    # Convert hex to bytes
    byte_data = binascii.unhexlify(hex_data)

    # Decode JPEG to a numpy array
    image_data = cv2.imdecode(np.frombuffer(byte_data, dtype=np.uint8), cv2.IMREAD_COLOR)

    # Rotate the image by 180 degrees
    image_data = cv2.rotate(image_data, cv2.ROTATE_180)

    # Display using OpenCV
    cv2.imshow('JPEG Image', image_data)
    cv2.waitKey(1) # 1 millisecond delay to allow the window to refresh

# Function to display grayscale image
def display_image(hex_data):
    # Convert hex to bytes
    byte_data = binascii.unhexlify(hex_data)

    # Convert byte array to numpy array
    image_data = np.frombuffer(byte_data, dtype=np.uint8)

    # Reshape to 2D array based on the given width
    HEIGHT = len(image_data) // WIDTH
    image_data = image_data.reshape((HEIGHT, WIDTH))

    # Display using OpenCV
    cv2.imshow('Grayscale Image', image_data)
    cv2.waitKey(1)

try:
    # Continuously read lines from the Arduino
    while True:
        if ser.in_waiting:
            # Read a line from the serial port
            line = ser.readline().decode('utf-8').strip()
            print(line)

            # Check if the line starts with "GRAY: "
            if line.startswith("GRAY: "):
                # Extract the hex data
                hex_data = line[6:]

                # Display the image
                display_image(hex_data)

            if line.startswith("JPEG: "):
                hex_data = line[6:] # Remove the "JPEG: " prefix
                display_jpeg(hex_data)

except KeyboardInterrupt:
    # Close the serial connection if the user interrupts the program
    ser.close()
    cv2.destroyAllWindows()
    print("\nSerial connection closed.")


