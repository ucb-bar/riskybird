
/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <JPEGDEC.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_spi_flash.h"
#include "esp_system.h"
#include "esp_timer.h"

#include "app_camera_esp.h"
#include "esp_camera.h"
#include "model_settings.h"
#include "image_provider.h"
#include "esp_main.h"

#include "Adafruit_VC0706.h"


static const char* TAG = "app_camera";

static uint16_t *display_buf; // buffer to hold data to be sent to display

#define RAW_WIDTH 160
#define RAW_HEIGHT 120




HardwareSerial cameraconnection(1);
Adafruit_VC0706 cam = Adafruit_VC0706(&cameraconnection);
JPEGDEC jpeg;



uint8_t grayscale[RAW_WIDTH*RAW_HEIGHT];



int saveImgBuf(JPEGDRAW *pDraw)
{
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

void scaleImg(int image_width, int image_height, int8_t * buf) {
  float scaleW = (float)RAW_WIDTH / image_width;
  float scaleH = (float)RAW_HEIGHT / image_height;

  float dominant_scale = scaleW > scaleH ? scaleW : scaleH;

  int cropped_width = RAW_WIDTH / dominant_scale;
  int cropped_height = RAW_HEIGHT / dominant_scale;

  // Calculate offsets to center the cropped portion
  int offsetX = (RAW_WIDTH - cropped_width * dominant_scale) * 0.5;
  int offsetY = (RAW_HEIGHT - cropped_height * dominant_scale) * 0.5;

  for(int y = 0; y < image_height; y++) {
    for(int x = 0; x < image_width; x++) {
      int sourceX = x * dominant_scale + offsetX;
      int sourceY = y * dominant_scale + offsetY;
      
      // Map the source pixel to the destination pixel
      buf[y * image_width + x] = grayscale[sourceY * RAW_WIDTH + sourceX] ^ 0x80;
    }
  }
}

// Get the camera module ready
TfLiteStatus InitCamera(tflite::ErrorReporter* error_reporter) {
// if display support is present, initialise display buf
#if DISPLAY_SUPPORT
  if (display_buf == NULL) {
    display_buf = (uint16_t *) heap_caps_malloc(96 * 2 * 96 * 2 * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  }
  if (display_buf == NULL) {
    ESP_LOGE(TAG, "Couldn't allocate display buffer");
    return kTfLiteError;
  }
#endif

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
  delay(500);
  
  

  // You can read the size back from the camera (optional, but maybe useful?)
  uint8_t imgsize = cam.getImageSize();
  Serial.print("Image size: ");
  Serial.print(imgsize);
  if (imgsize == VC0706_640x480) Serial.println(" (640x480)");
  else if (imgsize == VC0706_320x240) Serial.println(" (320x240)");
  else if (imgsize == VC0706_160x120) Serial.println(" (160x120)");
  else Serial.println(" (Unexpected Size)");

  cam.setCompression(36);
  delay(500);

  int ret = 0;
  if (ret != 0) {
    TF_LITE_REPORT_ERROR(error_reporter, "Camera init failed\n");
    return kTfLiteError;
  }
  TF_LITE_REPORT_ERROR(error_reporter, "Camera Initialized\n");
  return kTfLiteOk;
}

void *image_provider_get_display_buf()
{
  return (void *) display_buf;
}

// Get an image from the camera module
TfLiteStatus GetImage(tflite::ErrorReporter* error_reporter, int image_width,
                      int image_height, int channels, int8_t* image_data) {
  if (! cam.takePicture())  {
    Serial.println("Failed to snap!");
    ESP_LOGE(TAG, "Camera capture failed");
    return kTfLiteError;
  }
  else {
    Serial.println("Picture taken!");
  }
  


  // Get the size of the image (frame) taken  
  uint32_t jpglen = cam.frameLength();
  uint32_t jpgsize = jpglen;
  Serial.print("Storing ");
  Serial.print(jpglen, DEC);
  Serial.print(" byte image.");

  int32_t time = millis();
  byte wCount = 0; // For counting # of writes
  uint8_t *buffer;
  while (jpglen > 0) {
    // read 32 bytes at a time;
    
    uint32_t bytesToRead = min((uint32_t)1024*8, jpglen); // change 32 to 64 for a speedup but may not work with all setups!
    buffer = cam.readPicture(bytesToRead);

    if(++wCount >= 64) { // Every 2K, give a little feedback so it doesn't appear locked up
      //Serial.print('.');
      wCount = 0;
      
    }
    //Serial.print("Read ");  Serial.print(bytesToRead, DEC); Serial.println(" bytes");
    jpglen -= bytesToRead;
  }

  time = millis() - time;
  Serial.println("done!");
  Serial.print(time); Serial.println(" ms elapsed");
  

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
    }
    Serial.println();
    Serial.println("=====================");
  }

  cam.resumeVideo();
  delay(100);


  TF_LITE_REPORT_ERROR(error_reporter, "Image Captured\n");
  // We have initialised camera to grayscale
  // Just quantize to int8_t
  scaleImg(image_width, image_height, image_data);

  // TODO
  // esp_camera_fb_return(fb);
  /* here the esp camera can give you grayscale image directly */
  return kTfLiteOk;
}
