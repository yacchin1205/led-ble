#include "ble-metadata.h"

bool writeMetadataCharacteristic(Metadata* metadata, BLECharacteristic* metadataCharacteristic) {
    uint8_t buffer[MAX_METADATA_CHARACTERISTIC_SIZE];
    size_t size = metadata->writeTo(buffer, MAX_METADATA_CHARACTERISTIC_SIZE);
    if (size == 0) {
        return false;
    }
    metadataCharacteristic->writeValue(buffer, size);
    return true;
}