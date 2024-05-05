#include "ble.h"

bool writeStatesCharacteristic(
  States* states, BLECharacteristic* definitionCharacteristic
) {
  uint8_t buffer[MAX_DEFINITION_CHARACTERISTIC_SIZE];

  animation_characteristic_header_t header = {ANIMATION_CHARACTERISTIC_TYPE_STATES};
  memcpy(buffer, &header, sizeof(animation_characteristic_header_t));

  size_t size = states->writeHeaderTo(
    buffer + sizeof(animation_characteristic_header_t),
    MAX_DEFINITION_CHARACTERISTIC_SIZE - sizeof(animation_characteristic_header_t)
  );
  definitionCharacteristic->writeValue(
    buffer,
    size + sizeof(animation_characteristic_header_t)
  );
  return true;
}

bool readStatesCharacteristic(
  BLECharacteristic* definitionCharacteristic,
  const uint8_t* buffer, size_t size,
  States** states
) {
  if (states == NULL) {
    Serial.println("readStatesCharacteristic: states is NULL");
    return false;
  }
  *states = States::readFrom(buffer, size);
  if (*states == NULL) {
    Serial.println("readStatesCharacteristic: failed to read states");
    return false;
  }
  // Write the new states to the characteristic
  return writeStatesCharacteristic(*states, definitionCharacteristic);
}

bool writeAnimationCharacteristic(
  Animation* animation, unsigned char from_state, unsigned char to_state, BLECharacteristic* definitionCharacteristic
) {
  uint8_t buffer[MAX_DEFINITION_CHARACTERISTIC_SIZE];
  animation_characteristic_header_t header = {ANIMATION_CHARACTERISTIC_TYPE_ANIMATION};
  memcpy(buffer, &header, sizeof(animation_characteristic_header_t));
  states_header_t states_header = {0, 0, 1};
  memcpy(buffer + sizeof(animation_characteristic_header_t), &states_header, sizeof(states_header_t));
  size_t offset = sizeof(animation_characteristic_header_t) + sizeof(states_header_t);
  if (animation == NULL) {
    Serial.printf("writeAnimationCharacteristic: animation not found: %d -> %d\n", from_state, to_state);
    animation_header_t no_animation_header = {from_state, to_state};
    memcpy(buffer + offset, &no_animation_header, sizeof(animation_header_t));
    offset += sizeof(animation_header_t);
    channel_collection_header_t no_channels_header = {0};
    memcpy(buffer + offset, &no_channels_header, sizeof(channel_collection_header_t));
    offset += sizeof(channel_collection_header_t);
    definitionCharacteristic->writeValue(buffer, offset);
    return true;
  }
  size_t animation_size = animation->writeTo(NULL, 0);
  if (animation_size + offset > MAX_DEFINITION_CHARACTERISTIC_SIZE) {
    Serial.println("writeAnimationCharacteristic: buffer size is too small");
    return false;
  }
  animation_size = animation->writeTo(buffer + offset, animation_size);
  if (animation_size == 0) {
    Serial.println("writeAnimationCharacteristic: buffer size is too small");
    return false;
  }
  Serial.printf("writeAnimationCharacteristic: animation found: %d -> %d\n", from_state, to_state);
  definitionCharacteristic->writeValue(buffer, animation_size + offset);
  return true;
}

bool readAnimationCharacteristic(
  BLECharacteristic* definitionCharacteristic,
  const uint8_t* buffer, size_t size,
  Animation** animation
) {
  if (animation == NULL) {
    Serial.println("readAnimationCharacteristic: animation is NULL");
    return false;
  }
  *animation = Animation::readFrom(buffer, size, NULL);
  if (*animation == NULL) {
    Serial.println("readAnimationCharacteristic: failed to read animation");
    return false;
  }
  // Write the new animation to the characteristic
  return writeAnimationCharacteristic(
    *animation,
    (*animation)->from_state,
    (*animation)->to_state,
    definitionCharacteristic
  );
}

bool updateStatesCharacteristic(
  States* states,
  BLECharacteristic* definitionCharacteristic,
  const animation_characteristic_header_t* header,
  const uint8_t* buffer, size_t size,
  States** newStates
) {
  if ((header->type & ANIMATION_CHARACTERISTIC_MODE_MASK) == ANIMATION_CHARACTERISTIC_MODE_READ) {
    Serial.printf("updateAnimationsCharacteristic: read command\n");
    return writeStatesCharacteristic(states, definitionCharacteristic);
  }
  if ((header->type & ANIMATION_CHARACTERISTIC_MODE_MASK) != ANIMATION_CHARACTERISTIC_MODE_WRITE) {
    Serial.printf("updateAnimationsCharacteristic: unknown mode: %x\n", header->type);
    return false;
  }
  Serial.printf("updateAnimationsCharacteristic: write command\n");
  return readStatesCharacteristic(definitionCharacteristic, buffer, size, newStates);
}

bool checkStatesHeaderForAnimationRequest(const states_header_t* states_header) {
  if (states_header->delay_msec != 0) {
    Serial.printf("checkStatesHeaderForAnimationRequest: invalid delay_msec: %d\n", states_header->delay_msec);
    return false;
  }
  if (states_header->initial_state != 0) {
    Serial.printf("checkStatesHeaderForAnimationRequest: invalid initial_state: %d\n", states_header->initial_state);
    return false;
  }
  return true;
}

bool updateAnimationCharacteristic(
  States* states,
  BLECharacteristic* definitionCharacteristic,
  const animation_characteristic_header_t* header,
  const uint8_t* buffer,
  size_t size
) {
  if (size < sizeof(states_header_t) + sizeof(animation_header_t)) {
    Serial.printf("updateAnimationsCharacteristic: buffer size is too small: %d\n", size);
    return false;
  }
  const states_header_t *states_header = (const states_header_t *)buffer;
  if (!checkStatesHeaderForAnimationRequest(states_header)) {
    return false;
  }
  if (states_header->num_animations != 1) {
    Serial.printf("updateAnimationsCharacteristic: invalid num_animations: %d\n", states_header->num_animations);
    return false;
  }
  if ((header->type & ANIMATION_CHARACTERISTIC_MODE_MASK) == ANIMATION_CHARACTERISTIC_MODE_READ) {
    const animation_header_t *animation_header = (const animation_header_t *)(buffer + sizeof(states_header_t));
    Animation* animation = states->getAnimation(animation_header->from_state, animation_header->to_state);

    Serial.println("updateAnimationsCharacteristic: read command");
    return writeAnimationCharacteristic(
      animation, animation_header->from_state, animation_header->to_state, definitionCharacteristic
    );
  }
  if ((header->type & ANIMATION_CHARACTERISTIC_MODE_MASK) != ANIMATION_CHARACTERISTIC_MODE_WRITE) {
    Serial.printf("updateAnimationsCharacteristic: unknown mode: %x\n", header->type);
    return false;
  }
  Serial.printf("updateAnimationsCharacteristic: write command\n");
  Animation* newAnimation = NULL;
  bool r = readAnimationCharacteristic(
    definitionCharacteristic, buffer + sizeof(states_header_t), size - sizeof(states_header_t), &newAnimation
  );
  if (!r) {
    Serial.println("updateAnimationsCharacteristic: failed to read animation");
    return false;
  }
  states->replace(newAnimation);
  return true;
}

bool updateDefinitionCharacteristic(States* states, BLECharacteristic* definitionCharacteristic, States** newStates) {
  uint8_t buffer[MAX_DEFINITION_CHARACTERISTIC_SIZE];
  size_t size = definitionCharacteristic->valueLength();
  definitionCharacteristic->readValue(buffer, size);
  animation_characteristic_header_t *header = (animation_characteristic_header_t *)buffer;

  if ((header->type & ANIMATION_CHARACTERISTIC_TYPE_MASK) == ANIMATION_CHARACTERISTIC_TYPE_STATES) {
    Serial.println("updateAnimationsCharacteristic: states type");
    return updateStatesCharacteristic(
      states,
      definitionCharacteristic,
      header,
      buffer + sizeof(animation_characteristic_header_t),
      size - sizeof(animation_characteristic_header_t),
      newStates
    );
  }
  if ((header->type & ANIMATION_CHARACTERISTIC_TYPE_MASK) == ANIMATION_CHARACTERISTIC_TYPE_ANIMATION) {
    Serial.println("updateAnimationsCharacteristic: animation type");
    return updateAnimationCharacteristic(
      states,
      definitionCharacteristic,
      header,
      buffer + sizeof(animation_characteristic_header_t),
      size - sizeof(animation_characteristic_header_t)
    );
  }
  Serial.printf("updateAnimationsCharacteristic: unknown type: %x\n", header->type);
  return false;
}
