#include "ble.h"

bool writeAnimationCharacteristicHeader(States* states, BLECharacteristic* animationsCharacteristic) {
  unsigned char count = states->getAnimationCount();
  animation_characteristic_header_t header = {ANIMATION_CHARACTERISTIC_TYPE_HEADER, count};
  uint8_t buffer[MAX_ANIMATIONS_CHARACTERISTIC_SIZE];
  memcpy(buffer, &header, sizeof(animation_characteristic_header_t));
  size_t offset = sizeof(animation_characteristic_header_t);
  for (int i = 0; i < count; i++) {
    Animation* animation = states->getAnimation(i);
    size_t size = animation->writeHeaderTo(buffer + offset, MAX_ANIMATIONS_CHARACTERISTIC_SIZE - offset);
    if (size == 0) {
      Serial.println("writeAnimationCharacteristicHeader: buffer size is too small");
      return false;
    }
    offset += size;
  }
  animationsCharacteristic->writeValue(buffer, offset);
  return true;
}

bool writeAnimationCharacteristicData(States* states, BLECharacteristic* animationsCharacteristic, unsigned char from_state, unsigned char to_state) {
  Animation* animation = states->getAnimation(from_state, to_state);
  uint8_t buffer[MAX_ANIMATIONS_CHARACTERISTIC_SIZE];
  animation_characteristic_header_t header = {ANIMATION_CHARACTERISTIC_TYPE_ANIMATION, 1};
  memcpy(buffer, &header, sizeof(animation_characteristic_header_t));
  states_header_t states_header = {0, 0, 1};
  memcpy(buffer + sizeof(animation_characteristic_header_t), &states_header, sizeof(states_header_t));
  size_t offset = sizeof(animation_characteristic_header_t) + sizeof(states_header_t);
  if (animation == NULL) {
    Serial.printf("writeAnimationCharacteristicData: animation not found: %d -> %d\n", from_state, to_state);
    animation_header_t no_animation_header = {from_state, to_state};
    memcpy(buffer + offset, &no_animation_header, sizeof(animation_header_t));
    offset += sizeof(animation_header_t);
    channel_collection_header_t no_channels_header = {0};
    memcpy(buffer + offset, &no_channels_header, sizeof(channel_collection_header_t));
    offset += sizeof(channel_collection_header_t);
    animationsCharacteristic->writeValue(buffer, offset);
    return true;
  }
  size_t animation_size = animation->writeTo(NULL, 0);
  if (animation_size + offset > MAX_ANIMATIONS_CHARACTERISTIC_SIZE) {
    Serial.println("writeAnimationCharacteristicData: buffer size is too small");
    return false;
  }
  animation_size = animation->writeTo(buffer + offset, animation_size);
  if (animation_size == 0) {
    Serial.println("writeAnimationCharacteristicData: buffer size is too small");
    return false;
  }
  Serial.printf("writeAnimationCharacteristicData: animation found: %d -> %d\n", from_state, to_state);
  animationsCharacteristic->writeValue(buffer, animation_size + offset);
  return true;
}

bool updateAnimationsCharacteristic(States* states, BLECharacteristic* animationsCharacteristic) {
  unsigned char buffer[MAX_ANIMATIONS_CHARACTERISTIC_SIZE];
  size_t size = animationsCharacteristic->valueLength();
  animationsCharacteristic->readValue(buffer, size);
  animation_characteristic_header_t *header = (animation_characteristic_header_t *)buffer;
  if (header->type & ANIMATION_CHARACTERISTIC_TYPE_MASK == ANIMATION_CHARACTERISTIC_TYPE_HEADER) {
    Serial.printf("updateAnimationsCharacteristic: write header: %x\n", header->count);
    return writeAnimationCharacteristicHeader(states, animationsCharacteristic);
  }
  if (header->type & ANIMATION_CHARACTERISTIC_TYPE_MASK != ANIMATION_CHARACTERISTIC_TYPE_ANIMATION) {
    Serial.printf("updateAnimationsCharacteristic: unknown type: %x\n", header->type);
    return false;
  }
  if (size < sizeof(animation_characteristic_header_t) + sizeof(states_header_t) + sizeof(animation_header_t)) {
    Serial.printf("updateAnimationsCharacteristic: buffer size is too small: %d\n", size);
    return false;
  }
  if (header->count != 1) {
    Serial.printf("updateAnimationsCharacteristic: invalid count: %d\n", header->count);
    return false;
  }
  states_header_t *states_header = (states_header_t *)(buffer + sizeof(animation_characteristic_header_t));
  if (states_header->delay_msec != 0) {
    Serial.printf("updateAnimationsCharacteristic: invalid delay_msec: %d\n", states_header->delay_msec);
    return false;
  }
  if (states_header->initial_state != 0) {
    Serial.printf("updateAnimationsCharacteristic: invalid initial_state: %d\n", states_header->initial_state);
    return false;
  }
  if (states_header->num_animations != 1) {
    Serial.printf("updateAnimationsCharacteristic: invalid num_animations: %d\n", states_header->num_animations);
    return false;
  }
  animation_header_t *animation_header = (animation_header_t *)(buffer + sizeof(animation_characteristic_header_t) + sizeof(states_header_t));
  if (header->type & ANIMATION_CHARACTERISTIC_MODE_MASK == ANIMATION_CHARACTERISTIC_MODE_READ) {
    Serial.println("updateAnimationsCharacteristic: read mode");
    return writeAnimationCharacteristicData(states, animationsCharacteristic, animation_header->from_state, animation_header->to_state);
  }
  Serial.printf("updateAnimationsCharacteristic: unknown mode: %x\n", header->type);
  return false;
}
