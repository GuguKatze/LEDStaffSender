#include "Globals.h"
#include "ImuLogic.h"
#include <Arduino.h>
#include <ArduinoBLE.h>
#include <AudioAnalyzer.h>

Madgwick filter;
const float sensorRate = 104.00;
unsigned long microsPerReading = 0;
unsigned long microsPrevious = 0;

Analyzer Audio = Analyzer(9, 10, 0, 1); // strobe/yellow/10, reset/green/11, measure/blue/0

// ultrasonic
#define PIN_TRIGGER 2
#define PIN_ECHO    3
unsigned long lastSonicTime = 0;
const int SENSOR_MAX_RANGE = 300; // in cm
unsigned long duration;
unsigned int distance;

unsigned long lastVuTime = 0;
union vuPacket_ vuPacket;

unsigned long lastPitchPacketTime = 0;
union pitchPacket_ pitchPacket;

uint8_t peaks = 0;
unsigned long lastVuStats = 0;
uint8_t vuCount = 0;

uint8_t ledRed   = 4;
uint8_t ledGreen = 2;
uint8_t ledBlue  = 3;

void setup() {

  // statusLED
  pinMode(  ledRed, OUTPUT);
  pinMode(ledGreen, OUTPUT);
  pinMode( ledBlue, OUTPUT);
  analogWrite(  ledRed, 24);
  analogWrite(ledGreen,  0);
  analogWrite( ledBlue,  0);
  
  analogReadResolution(12);
  //analogReadResolution(8);
  delay(1000);
  Serial.begin(9600);
  //Serial.begin(115200);
  Audio.Init();

  // IMU
  while(!IMU.begin()) { delay(1000); };
  filter.begin(sensorRate);
  microsPerReading = 1000000 / sensorRate;

  // BLE
  BLE.begin();
  String address = BLE.address();
  Serial.print("BLE Center @ ");
  Serial.println(address);
  BLE.scanForUuid("19b10000-e8f2-537e-4f6c-d104768a1214");
}

void loop() {
  BLEDevice peripheral = BLE.available();
  if (peripheral) {
    Serial.print("Found ");
    Serial.print(peripheral.address());
    Serial.print(" '");
    Serial.print(peripheral.localName());
    Serial.print("' ");
    Serial.print(peripheral.advertisedServiceUuid());
    Serial.println();
    if (peripheral.localName() != "LED") {
      return;
    }
    BLE.stopScan();
    controlLed(peripheral);
    BLE.scanForUuid("19b10000-e8f2-537e-4f6c-d104768a1214");
  }
}

void controlLed(BLEDevice peripheral) {
  Serial.println("Connecting ...");
  if (peripheral.connect()) {
    Serial.println("Connected");
  } else {
    Serial.println("Failed to connect!");
    return;
  }
  Serial.println("Discovering attributes ...");
  if (peripheral.discoverAttributes()) {
    Serial.println("Attributes discovered");
  } else {
    Serial.println("Attribute discovery failed!");
    peripheral.disconnect();
    return;
  }
  BLECharacteristic ledCharacteristic = peripheral.characteristic("19b10001-e8f2-537e-4f6c-d104768a1214");
  if (!ledCharacteristic) {
    Serial.println("Peripheral does not have LED characteristic!");
    peripheral.disconnect();
    return;
  } else if (!ledCharacteristic.canWrite()) {
    Serial.println("Peripheral does not have a writable LED characteristic!");
    peripheral.disconnect();
    return;
  }
  while (peripheral.connected()) {
    analogWrite(  ledRed,  0);
    ///
    /// IMU
    ///
    unsigned long microsNow = micros();
    if(microsNow - microsPrevious >= microsPerReading) {
      imuLogic();
      if(millis() - lastPitchPacketTime > 250){
        analogWrite(ledGreen,  0);
        analogWrite( ledBlue, 24);
        lastPitchPacketTime = millis();
        uint8_t p = int(pitch);
        pitchPacket.pitch = p;
        Serial.println(p);
        ledCharacteristic.writeValue(pitchPacket.bytes, sizeof(pitchPacket.bytes));
      }
    }

    ///
    /// VU
    ///
    if(millis() - lastVuTime > 50){
      lastVuTime = millis();
      Audio.ReadFreq(vuPacket.left, vuPacket.right);

      for(int i=0; i<5; i++){ // skip the highest 2 bands due to potential white noise
        if (vuPacket.left[i] > 30 && peaks < 64){ peaks++; }
        if(vuPacket.right[i] > 30 && peaks < 64){ peaks++; }
      }
      if(peaks > 16){
         analogWrite(ledGreen,  0);
         analogWrite( ledBlue, 24);
         // send via BLE
         ledCharacteristic.writeValue(vuPacket.bytes, sizeof(vuPacket.bytes));
         vuCount++;
         if(millis() - lastVuStats > 1000){
            lastVuStats = millis();
            //Serial.println("vuStats: " + String(vuCount));
            vuCount = 0;
         }
      }else{
        analogWrite(ledGreen, 24);
        analogWrite( ledBlue,  0);
      }
      if(peaks > 0){
        peaks--;
      }
      
      /*
      for(int i=0;i<7;i++){
        Serial.print(max((vu.left[i]),0));
        Serial.print(":");
        Serial.print(max((vu.right[i]),0));
        if(i<6){ Serial.print(", "); }else{ Serial.println(); }
      }   
      */
      
    }
    ///
    ///
    ///
  }
  analogWrite(  ledRed, 24);
  analogWrite(ledGreen,  0);
  analogWrite( ledBlue,  0);
  Serial.println("Peripheral disconnected");
}
