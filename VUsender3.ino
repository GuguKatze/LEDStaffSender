#include "Globals.h"
#include "ImuLogic.h"
#include <Arduino.h>
#include <ArduinoBLE.h>
#include <AudioAnalyzer.h>

Madgwick filter;
const float sensorRate = 104.00;
unsigned long microsPerReading;
unsigned long microsPrevious = 0;

// ultrasonic
#define PIN_TRIGGER 2
#define PIN_ECHO    3
unsigned long lastSonicTime = 0;
const int SENSOR_MAX_RANGE = 300; // in cm
unsigned long duration;
unsigned int distance;

Analyzer Audio = Analyzer(9, 10, 0, 1); // strobe/yellow/10, reset/green/11, measure/blue/0

/////////////
// packets //
/////////////
union effectPacket_           effectPacket;
union pitchPacket_             pitchPacket;
union vuPacket_                   vuPacket;
union pitchRemotePacket_ pitchRemotePacket;

unsigned long lastPitchRemotePacketTime = 0;
unsigned long lastVuPacketTime          = 0;
unsigned long lastEffectTime            = 0;

uint8_t peaks = 0;

uint8_t ledRed   = 4;
uint8_t ledGreen = 2;
uint8_t ledBlue  = 3;
unsigned long ledRedLastTime = 0;
unsigned long ledGreenLastTime = 0;
unsigned long ledBlueLastTime = 0;


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
  BLECharacteristic      effectCharacteristic = peripheral.characteristic("19b10001-e8f2-537e-4f6c-d104768a1212");
  BLECharacteristic pitchRemoteCharacteristic = peripheral.characteristic("19b10001-e8f2-537e-4f6c-d104768a1213");
  BLECharacteristic          vuCharacteristic = peripheral.characteristic("19b10001-e8f2-537e-4f6c-d104768a1214");

  /*
  if (!ledCharacteristic) {
    Serial.println("Peripheral does not have LED characteristic!");
    peripheral.disconnect();
    return;
  } else if (!ledCharacteristic.canWrite()) {
    Serial.println("Peripheral does not have a writable LED characteristic!");
    peripheral.disconnect();
    return;
  }
  */
  
  while (peripheral.connected()) {
    if(millis() - ledRedLastTime   > 25) { analogWrite(    ledRed,  0); }
    if(millis() - ledGreenLastTime > 25) { analogWrite(  ledGreen,  0); }
    if(millis() - ledBlueLastTime  > 25) { analogWrite(   ledBlue,  0); }
    
    ////////////
    // EFFECT //
    ////////////
    if(millis() - lastEffectTime > 1000 * 20){
      lastEffectTime = millis();
      effectPacket.effect   = 1;
      effectPacket.duration = 5;
      analogWrite( ledRed, 24);
      ledRedLastTime = millis();
      effectCharacteristic.writeValue(effectPacket.bytes, sizeof(effectPacket.bytes));
    }
    /////////
    // IMU //
    /////////
    if(micros() - microsPrevious >= microsPerReading) {
      microsPrevious = micros();
      imuLogic();
      if(millis() - lastPitchRemotePacketTime > 250){
        lastPitchRemotePacketTime = millis();
        analogWrite( ledGreen, 24);
        ledBlueLastTime = millis();
        int8_t pitchRemoteFilteredInt = int(pitchRemoteFiltered);
        pitchRemotePacket.pitch = pitchRemoteFilteredInt;
        pitchRemoteCharacteristic.writeValue(pitchRemotePacket.bytes, sizeof(pitchRemotePacket.bytes));
      }
    }

    ////////
    // VU //
    ////////
    if(millis() - lastVuPacketTime > 3000){
      lastVuPacketTime = millis();
      Audio.ReadFreq(vuPacket.left, vuPacket.right);
      for(int i=0; i<5; i++){ // skip the highest 2 bands due to potential white noise
        if (vuPacket.left[i] > 30 && peaks < 64){ peaks++; }
        if(vuPacket.right[i] > 30 && peaks < 64){ peaks++; }
      }
      if(peaks > 16){
         analogWrite( ledBlue, 24);
         ledBlueLastTime = millis();
         vuCharacteristic.writeValue(vuPacket.bytes, sizeof(vuPacket.bytes));
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
