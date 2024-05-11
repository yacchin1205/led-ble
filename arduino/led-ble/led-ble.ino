#include <ArduinoBLE.h>
#include "states.h"
#include "metadata.h"
#include "sinks.h"
#include "ble.h"

BLEService ledService("19B10000-E8F2-537E-4F6C-D104768A1214"); // Bluetooth® Low Energy LED Service

// unsigned char animationsCharacteristicBuffer[MAX_ANIMATIONS_CHARACTERISTIC_SIZE];
BLEByteCharacteristic switchCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);
BLECharacteristic metadataCharacteristic("19B10002-E8F2-537E-4F6C-D104768A1215", BLERead, MAX_METADATA_CHARACTERISTIC_SIZE);
BLECharacteristic definitionCharacteristic("19B10002-E8F2-537E-4F6C-D104768A1216", BLERead | BLEWrite, MAX_DEFINITION_CHARACTERISTIC_SIZE);

States* states = NULL;
Metadata* metadata = NULL;
ChannelSinks* sinks = NULL;

#include "custom.h"

void setup() {
  Serial.begin(115200);

  setMetadata();
  sinks = new ChannelSinks();
  setSinks();
  states = new States(0);
  setAnimations();

  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");

    while (1);
  }

  BLE.setLocalName(DEVICE_NAME);
  BLE.setAdvertisedService(ledService);

  ledService.addCharacteristic(definitionCharacteristic);
  ledService.addCharacteristic(switchCharacteristic);
  ledService.addCharacteristic(metadataCharacteristic);

  BLE.addService(ledService);

  writeStatesCharacteristic(states, &definitionCharacteristic);
  switchCharacteristic.writeValue(0);
  if (metadata != NULL) {
    writeMetadataCharacteristic(metadata, &metadataCharacteristic);
  } else {
    Serial.println("No metadata found");
  }

  BLE.advertise();

  Serial.println("BLE LED Peripheral");
}

void loop() {
  BLEDevice central = BLE.central();

  if (!central) {
    states->loop(sinks);
    return;
  }
  Serial.print("Connected to central: ");
  Serial.println(central.address());

  while (central.connected()) {
    if (definitionCharacteristic.written()) {
      States* newStates = NULL;
      bool r = updateDefinitionCharacteristic(states, &definitionCharacteristic, &newStates);
      if (!r) {
        Serial.println("Failed to update animations characteristic");
      }
      if (newStates != NULL) {
        delete states;
        states = newStates;
      }
    }
    if (switchCharacteristic.written()) {
      int value = switchCharacteristic.value();
      Serial.printf("Next state: %d\n", value);
      states->reserveState(value);
    }
    states->loop(sinks);
  }

  Serial.print(F("Disconnected from central: "));
  Serial.println(central.address());
}
