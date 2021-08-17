#include <Arduino.h>
#include <ArduinoBLE.h>
#include <AudioAnalyzer.h>
Analyzer Audio = Analyzer(9, 10, 0, 1); // strobe/yellow/10, reset/green/11, measure/blue/0

union vu_ {
  struct __attribute__((packed)){
    //uint16_t left[7];
    uint8_t left[7];
    //uint16_t right[7];
    uint8_t right[7];
  };
  //uint8_t bytes[28];
  uint8_t bytes[14];
};
union vu_ vu;

unsigned long lastVuTime = 0;

void setup() {
  analogReadResolution(12);
  //analogReadResolution(8);
  delay(1000);
  Serial.begin(115200);
  Audio.Init();
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
    if(millis() - lastVuTime > 10){
      Audio.ReadFreq(vu.left, vu.right);
      /*
      for(int i=0;i<7;i++){
        Serial.print(max((vu.left[i]),0));
        Serial.print(":");
        Serial.print(max((vu.right[i]),0));
        if(i<6){ Serial.print(", "); }else{ Serial.println(); }
      }
      */
      Serial.println(">");
      ledCharacteristic.writeValue(vu.bytes, sizeof(vu.bytes));
      lastVuTime = millis();
    }
  }
  Serial.println("Peripheral disconnected");
}
