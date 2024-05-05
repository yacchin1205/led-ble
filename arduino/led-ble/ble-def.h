#pragma once
#include "Arduino.h"
#include <ArduinoBLE.h>
#include "states.h"

#define MAX_DEFINITION_CHARACTERISTIC_SIZE 512

#define ANIMATION_CHARACTERISTIC_TYPE_MASK 0xF
#define ANIMATION_CHARACTERISTIC_TYPE_STATES 0x1
#define ANIMATION_CHARACTERISTIC_TYPE_ANIMATION 0x2
#define ANIMATION_CHARACTERISTIC_MODE_MASK 0xF0
#define ANIMATION_CHARACTERISTIC_MODE_READ 0x10
#define ANIMATION_CHARACTERISTIC_MODE_WRITE 0x20
#define ANIMATION_CHARACTERISTIC_MODE_DELETE 0x40

typedef struct __animation_characteristic_header_t {
  uint8_t type;
} animation_characteristic_header_t;

bool writeStatesCharacteristic(States* states, BLECharacteristic* animationsCharacteristic);
bool updateDefinitionCharacteristic(States* states, BLECharacteristic* animationsCharacteristic, States** newStates);
