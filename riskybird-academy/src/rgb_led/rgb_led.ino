// Include the necessary library to control the built-in LED.
#include <Adafruit_NeoPixel.h>

// Define some constants for our NeoPixel LED.
#define PIN 8          // The pin where the NeoPixel is connected.
#define NUMPIXELS 1    // The number of pixels (in this case, just one).
// TODO: Define the BRIGHTNESS constant with a value between 0 (off) and 255 (full brightness).

// Create a NeoPixel object called 'pixels'.
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  // This function runs once when the board starts up.

  // Initialize the NeoPixel library.
  pixels.begin();
  
  // Set the brightness of the NeoPixel.
  pixels.setBrightness(BRIGHTNESS);
  
  // Clear any existing color on the NeoPixel.
  pixels.clear();
}

// Custom function to set the color of the NeoPixel.
void setColor(int r, int g, int b) {
  pixels.setPixelColor(0, pixels.Color(r, g, b)); // Set the color.
  pixels.show(); // This command pushes the set color to the NeoPixel.
}

void loop() {
  // This function keeps running repeatedly.

  // TODO: Set the NeoPixel color to red using the setColor function.
  delay(1000); // Wait for a second.
  
  // TODO: Set the NeoPixel color to green using the setColor function.
  delay(1000); // Wait for a second.
  
  // TODO: Set the NeoPixel color to blue using the setColor function.
  delay(1000); // Wait for a second.
}


