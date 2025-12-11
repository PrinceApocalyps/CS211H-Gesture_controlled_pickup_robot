#include <Arduino.h>
#include <iostream>
#include <string>
#include <cmath>
#include <Arduino_LSM6DS3.h>
#include <ArduinoBLE.h>
//#include <neuton.h>

#include <Imu.h>

// define sample rate and delay
#define SAMPLE_RATE 100
#define DELAY (1000.0 / (float) SAMPLE_RATE) 

// Threshold for gyroscope sum (X+Y+Z) magnitude to trigger recording
float threshold = 200.0; 

// 2 seconds of data at SAMPLE_RATE
uint32_t numSamples = 2 * SAMPLE_RATE; 
uint32_t samplesRead = numSamples;


// BLE Service and Characteristics - using custom 128-bit UUIDs
BLEService imuService("19B10000-E8F2-537E-4F6C-D104768A1214");

// Characteristics for pitch and roll (as strings)
BLEStringCharacteristic pitchCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify, 20);
BLEStringCharacteristic rollCharacteristic("19B10002-E8F2-537E-4F6C-D104768A1214",BLERead | BLENotify, 20);

Imu myImu;

void setup() {
  Serial.begin(115200);

  // Wait up to 3 seconds for Serial to open (optional)
  unsigned long start = millis();
  while(!Serial && millis() - start < 3000);

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
    }
  
  
  // Start BLE
  if (!BLE.begin()) {
    Serial.println("Starting Bluetooth® Low Energy failed!");
    while (1);
  }
  Serial.println("BLE initialized");

  // Set the device name shown in Bluetooth scan lists
  BLE.setLocalName("peripheral_rp2040");
  
  // Set up the BLE service and characteristics
  BLE.setAdvertisedService(imuService);
  imuService.addCharacteristic(pitchCharacteristic);
  imuService.addCharacteristic(rollCharacteristic);
  BLE.addService(imuService);

  Serial.println("Service and characteristics added");

  // Set initial values
  pitchCharacteristic.writeValue("0.0");
  rollCharacteristic.writeValue("0.0");

  Serial.println("Initial values set");

  // Start advertising
  BLE.advertise();

  Serial.println("Advertising peripheral_rp2040: ");
  Serial.println("Waiting for connections...");

}

void loop() {
  // Listen for BLE centrals to connect
  BLEDevice central = BLE.central();
  
  if (central) {
    Serial.print("Connected to central: ");
    Serial.println(central.address());
    
    // While connected, update characteristics
    while (central.connected()) {
      // Poll BLE events
      //central.poll();

      myImu.update_data();
      
      myImu.print_accel();
      Serial.print("Pitch angle = ");
      Serial.println(myImu.getPitch());
      Serial.print("Roll angle = ");
      Serial.println(myImu.getRoll());
      Serial.println();
      
      // Convert float to string and update characteristics
      String pitchStr = String(myImu.getPitch(), 2);
      String rollStr = String(myImu.getRoll(), 2);

      pitchCharacteristic.writeValue(pitchStr);
      rollCharacteristic.writeValue(rollStr);
      
      delay(50);
    }
    
    // When disconnected
    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
  }
  
  delay(10);
}