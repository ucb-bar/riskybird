#include <Preferences.h>

Preferences preferences;

void setup() {
  Serial.begin(115200);
  
  // Open a namespace called "countData" for reading and writing
  preferences.begin("countData", false);
  
  // TODO 1: Retrieve the stored count value (if it exists) or set it to 0.
  // Preferences API: getInt(key (string), default);
  
  // TODO 2: Print the retrieved count value to the Serial Monitor.
  
  // TODO 3: Increment the count value.
  
  // TODO 4: Store the updated count value back to the NVS.
  // Preferences API: putInt(key, default);
  
  preferences.end(); // Close the Preferences
}

void loop() {
  // We don't need to continuously run any logic in this lab.
  delay(1000);
}
