#include <TensorFlowLite_ESP32.h>
/* Copyright 2020 The TensorFlow Authors. All Rights Reserved.

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


#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "model.h"

#define SERIAL_BUFFER_LEN 256
#define STATE_SIZE 4
#define ACTION_SIZE 2

// Globals, used for compatibility with Arduino-style sketches.
namespace {
tflite::ErrorReporter* error_reporter = nullptr;
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;
int inference_count = 0;
char buf[SERIAL_BUFFER_LEN];
constexpr int kTensorArenaSize = 2000;
uint8_t tensor_arena[kTensorArenaSize];
}  // namespace

void parseSerialObservation(float* obs) {
    char buf[SERIAL_BUFFER_LEN];
    int bufIndex = 0;  // Index to keep track of buffer position

    // Read from serial input until a newline is encountered or the buffer is full
    while (bufIndex < SERIAL_BUFFER_LEN - 1) {
        if (Serial.available()) {
            char inChar = (char)Serial.read();
            buf[bufIndex++] = inChar;

            // Break out of loop if a newline character is read
            if (inChar == '\n') {
                break;
            }
        }
    }
    buf[bufIndex] = '\0';  // Null terminate the buffer string

    // Check if the buffer contains the "obs:" string and extract the observations
    char* obsStart = strstr(buf, "obs:");
    if (obsStart != NULL) {
        sscanf(obsStart, "obs: [%f %f %f %f]", &obs[0], &obs[1], &obs[2], &obs[3]);
    }
}

// The name of this function is important for Arduino compatibility.
void setup() {
  // Initialize serial communication.
  Serial.begin(115200);

  // Set up error reporting for TensorFlow Lite.
  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;

  // Map the model into a usable data structure.
  model = tflite::GetModel(actor_quant_tflite);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    TF_LITE_REPORT_ERROR(error_reporter, "Model version mismatch.");
    return;
  }

  // Use all TensorFlow Lite operations.
  static tflite::AllOpsResolver resolver;

  // Initialize the TensorFlow Lite interpreter with model and resolver.
  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
  interpreter = &static_interpreter;

  // Allocate memory for model's tensors.
  if (interpreter->AllocateTensors() != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "AllocateTensors() failed");
    return;
  }

  // Pointers to model's input and output tensors.
  input = interpreter->input(0);
  output = interpreter->output(0);
}

// The name of this function is important for Arduino compatibility.
void loop() {
  // Array to store observation data.
  float  obs[STATE_SIZE];
  int8_t obs_quant[STATE_SIZE];

  // Array to store observation data.
  int8_t action_quantized[ACTION_SIZE];
  float action[ACTION_SIZE];
  uint8_t action_choice;

  // Parse observation data from serial input.
  parseSerialObservation(obs);
  
  // Quantize observations
  for(int i = 0; i < STATE_SIZE; i++) {
    // Quantize the input from floating-point to integer
    obs_quant[i] = obs[i] / input->params.scale + input->params.zero_point;
    // Place the quantized input in the model's input tensor
    input->data.int8[i] = obs_quant[i];
  }

  // Run inference, and report any error
  TfLiteStatus invoke_status = interpreter->Invoke();
  if (invoke_status != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "Invoke failed on obs: [%f %f %f %f]\n",
                         obs[0], obs[1], obs[2], obs[3]);
    return;
  }

  // Generate unquantized action data
  for (int i = 0; i < ACTION_SIZE; i++) {
    // Obtain the quantized output from model's output tensor
    action_quantized[i] = output->data.int8[i];
    // Dequantize the output from integer to floating-point
    action[i] = (action_quantized[i] - output->params.zero_point) * output->params.scale;
  } 

  // Choose action based on the model's prediction.
  action_choice = action[1] > action[0];
  
  // Send the chosen action over serial port.
  Serial.printf("action: [%d]\n", action_choice);
}
