// This is a basic snapshot sketch using the VC0706 library.
// On start, the Arduino will find the camera and SD card and
// then snap a photo, saving it to the SD card.
// Public domain.

// If using an Arduino Mega (1280, 2560 or ADK) in conjunction
// with an SD card shield designed for conventional Arduinos
// (Uno, etc.), it's necessary to edit the library file:
//   libraries/SD/utility/Sd2Card.h
// Look for this line:
//   #define MEGA_SOFT_SPI 0
// change to:
//   #define MEGA_SOFT_SPI 1
// This is NOT required if using an SD card breakout interfaced
// directly to the SPI bus of the Mega (pins 50-53), or if using
// a non-Mega, Uno-style board.

#include "Adafruit_VC0706.h"
#include <JPEGDEC.h>


#define RAW_WIDTH 160
#define RAW_HEIGHT 120
// #define DECODE

HardwareSerial cameraconnection(1);
Adafruit_VC0706 cam = Adafruit_VC0706(&cameraconnection);
JPEGDEC jpeg;


uint8_t grayscale[RAW_WIDTH*RAW_HEIGHT];


// Pins for camera connection are configurable.
// With the Arduino Uno, etc., most pins can be used, except for
// those already in use for the SD card (10 through 13 plus
// chipSelect, if other than pin 10).
// With the Arduino Mega, the choices are a bit more involved:
// 1) You can still use SoftwareSerial and connect the camera to
//    a variety of pins...BUT the selection is limited.  The TX
//    pin from the camera (RX on the Arduino, and the first
//    argument to SoftwareSerial()) MUST be one of: 62, 63, 64,
//    65, 66, 67, 68, or 69.  If MEGA_SOFT_SPI is set (and using
//    a conventional Arduino SD shield), pins 50, 51, 52 and 53
//    are also available.  The RX pin from the camera (TX on
//    Arduino, second argument to SoftwareSerial()) can be any
//    pin, again excepting those used by the SD card.
// 2) You can use any of the additional three hardware UARTs on
//    the Mega board (labeled as RX1/TX1, RX2/TX2, RX3,TX3),
//    but must specifically use the two pins defined by that
//    UART; they are not configurable.  In this case, pass the
//    desired Serial object (rather than a SoftwareSerial
//    object) to the VC0706 constructor.


// int saveImgBuf(JPEGDRAW *pDraw)
// {
//   Serial.printf("Draw pos = %d,%d. size = %d x %d\n", pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);
//   for(uint16_t i = 0; i < pDraw->iHeight; i++) {
//     uint16_t idx = (pDraw->y + i)*RAW_WIDTH + pDraw->x;
//     memcpy(&grayscale[idx], &(pDraw->pPixels[idx]), pDraw->iWidth * sizeof(uint8_t));
//   }
//   return 1; // returning true (1) tells JPEGDEC to continue decoding. Returning false (0) would quit decoding immediately.
// } /* drawMCUs() */

int saveImgBuf(JPEGDRAW *pDraw)
{
  Serial.printf("Draw pos = %d,%d. size = %d x %d\n", pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);
  for (uint16_t y = 0; y < pDraw->iHeight; y++) {
    uint16_t dest_idx = (pDraw->y + y) * RAW_WIDTH + pDraw->x;
    uint16_t src_idx = y * pDraw->iWidth;
    uint8_t * pixels_bytes = (uint8_t *) (pDraw->pPixels);
    // memcpy(&grayscale[dest_idx], &(pixels_bytes[src_idx]), pDraw->iWidth * sizeof(uint8_t));
    for(uint16_t x = 0; x < pDraw->iWidth; x++) {
      
      grayscale[RAW_WIDTH*RAW_HEIGHT - (dest_idx + x + 1)] = pixels_bytes[src_idx + x];
    }
  }
  return 1; // returning true (1) tells JPEGDEC to continue decoding. Returning false (0) would quit decoding immediately.
}

void setup() {
  Serial.begin(115200);
  Serial.println("VC0706 Camera snapshot test");
  
  // see if the card is present and can be initialized:

  
  // Try to locate the camera
  while(1) {
  delay(1000);
  if (cam.begin()) {
    Serial.println("Camera Found:");
    // while(1) {
    // while (cameraconnection.available())
    // Serial.write(cameraconnection.read());
    // }
    break;
  } else {
    Serial.println("No camera found?");
  }
  
  }
  delay(1000);
  // Print out the camera version information (optional)
  char *reply = cam.getVersion();
  if (reply == 0) {
    Serial.print("Failed to get version");
  } else {
    Serial.println("-----------------");
    Serial.print(reply);
    Serial.println("-----------------");
  }
  delay(1000);
  // Set the picture size - you can choose one of 640x480, 320x240 or 160x120 
  // Remember that bigger pictures take longer to transmit!
  
  
  Serial.println("Setting image size.");
  // cam.setImageSize(VC0706_640x480);        // biggest
  //cam.setImageSize(VC0706_320x240);        // medium
  cam.setImageSize(VC0706_160x120);          // small
  delay(1000);

  // You can read the size back from the camera (optional, but maybe useful?)
  uint8_t imgsize = cam.getImageSize();
  Serial.print("Image size: ");
  Serial.print(imgsize);
  if (imgsize == VC0706_640x480) Serial.println(" (640x480)");
  else if (imgsize == VC0706_320x240) Serial.println(" (320x240)");
  else if (imgsize == VC0706_160x120) Serial.println(" (160x120)");
  else Serial.println(" (Unexpected Size)");
  delay(500);
  
  cam.setCompression(36);
  delay(500);

  while(1) {
  // Serial.println("Snap in 3 secs...");
  // delay(3000);

  if (! cam.takePicture()) 
    Serial.println("Failed to snap!");
  else 
    Serial.println("Picture taken!");
  


  // Get the size of the image (frame) taken  
  uint32_t jpglen = cam.frameLength();
  uint32_t jpgsize = jpglen;
  Serial.print("Storing ");
  Serial.print(jpglen, DEC);
  Serial.print(" byte image.");

  int32_t time = millis();
  byte wCount = 0; // For counting # of writes
  Serial.println();
  #ifndef DECODE
    Serial.println("=====================");
    Serial.print("JPEG: ");
  #endif
  uint8_t *buffer;
  while (jpglen > 0) {
    // read 32 bytes at a time;
    
    uint32_t bytesToRead = min((uint32_t)1024*8, jpglen); // change 32 to 64 for a speedup but may not work with all setups!
    buffer = cam.readPicture(bytesToRead);
    
    for(int i =0; i < bytesToRead; i++) {
      #ifndef DECODE
        if (buffer[i] < 16) {    // Add leading zero for numbers less than 16 (0x0F instead of 0xF)
          Serial.print("0");
        }
        Serial.print(buffer[i], HEX);
      #endif
      //if(i % 1 == 0) { Serial.println(); }
    }
    if(++wCount >= 64) { // Every 2K, give a little feedback so it doesn't appear locked up
      //Serial.print('.');
      wCount = 0;
      
    }
    //Serial.print("Read ");  Serial.print(bytesToRead, DEC); Serial.println(" bytes");
    jpglen -= bytesToRead;
  }
  #ifndef DECODE
    Serial.println();
    Serial.println("=====================");
  #endif
  time = millis() - time;
  Serial.println("done!");
  Serial.print(time); Serial.println(" ms elapsed");
  
  #ifdef DECODE
  long lTime;
  char szTemp[64];

  if(jpeg.openRAM(buffer, jpgsize, saveImgBuf)) {
    Serial.println("Successfully opened JPEG image");
    Serial.printf("Image size: %d x %d, orientation: %d, bpp: %d\n", jpeg.getWidth(), jpeg.getHeight(), jpeg.getOrientation(), jpeg.getBpp());
    jpeg.setPixelType(EIGHT_BIT_GRAYSCALE);
    lTime = micros();
    if (jpeg.decode(0, 0, 0))
    {
      lTime = micros() - lTime;
      sprintf(szTemp, "Successfully decoded image in %d us", (int)lTime);
      Serial.println(szTemp);

      Serial.println("=====================");
      Serial.print("GRAY: ");
      for(uint16_t i =0; i < RAW_HEIGHT*RAW_WIDTH; i++) {
      if (grayscale[i] < 16) {    // Add leading zero for numbers less than 16 (0x0F instead of 0xF)
        Serial.print("0");
      }
      Serial.print(grayscale[i], HEX);
    }
    Serial.println();
    Serial.println("=====================");
    }
  }
  #endif
  cam.resumeVideo();
  delay(100);

  }
}

void loop() {
}

