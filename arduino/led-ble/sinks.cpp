
#include <Arduino.h>

#include "sinks.h"

AnalogWriteChannelSink::AnalogWriteChannelSink(uint8_t sink, uint8_t pin): ChannelSink(sink){
    this->pin = pin;
}

AnalogWriteChannelSink::~AnalogWriteChannelSink() {
}

void AnalogWriteChannelSink::setValue(int frame, float value) {
    // Serial.printf("LEDChannel(%d)::setValue(%f) for #%d\n", this->pin, value, frame);
    analogWrite(this->pin, value);
}

void AnalogWriteChannelSink::apply(int frame) {
}

#ifdef ENABLE_FASTLED

FastLEDChannelManager::FastLEDChannelManager(int num_leds) {
    this->num_leds = num_leds;
    this->leds = (CRGB*)malloc(sizeof(CRGB) * num_leds);
    memset(this->leds, 0, sizeof(CRGB) * num_leds);
}

FastLEDChannelManager::~FastLEDChannelManager() {
    if (this->leds != NULL) {
        free(this->leds);
        this->leds = NULL;
    }
}

CRGB* FastLEDChannelManager::getLeds() {
    return this->leds;
}

void FastLEDChannelManager::onApply(int frame) {
    // Serial.printf("FastLEDChannelManager::onApply for #%d\n", frame);
    FastLED.show();
}

FastLEDChannelSink::FastLEDChannelSink(uint8_t sink, FastLEDChannelManager* manager, int led, int channel): ChannelSink(sink) {
    this->manager = manager;
    this->led = led;
    this->channel = channel;
}

FastLEDChannelSink::~FastLEDChannelSink() {
}

void FastLEDChannelSink::setValue(int frame, float value) {
    // Serial.printf("FastLEDChannel(%d)::setValue(%f) for #%d\n", this->led, value, frame);
    this->manager->getLeds()[this->led][this->channel] = (int)value;
}

void FastLEDChannelSink::apply(int frame) {
    ;
}

#endif
