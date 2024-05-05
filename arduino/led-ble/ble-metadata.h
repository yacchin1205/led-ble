#pragma once
#include "Arduino.h"
#include <ArduinoBLE.h>
#include "metadata.h"

#define MAX_METADATA_CHARACTERISTIC_SIZE 512

bool writeMetadataCharacteristic(Metadata* metadata, BLECharacteristic* metadataCharacteristic);