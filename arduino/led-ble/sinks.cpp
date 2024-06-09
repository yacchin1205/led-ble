
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
    this->rgbs = (rgb_t*)malloc(sizeof(rgb_t) * num_leds);
    memset(this->rgbs, 0, sizeof(rgb_t) * num_leds);
    this->leds = (CRGB*)malloc(sizeof(CRGB) * num_leds);
    memset(this->leds, 0, sizeof(CRGB) * num_leds);
    this->dirty = false;
}

FastLEDChannelManager::~FastLEDChannelManager() {
    if (this->leds != NULL) {
        free(this->leds);
        this->leds = NULL;
    }
    if (this->rgbs != NULL) {
        free(this->rgbs);
        this->rgbs = NULL;
    }
}

CRGB* FastLEDChannelManager::getLeds() {
    return this->leds;
}

void FastLEDChannelManager::setLeds(int led, int channel, float value) {
    rgb_t* rgb = &this->rgbs[led];
    if (channel == 0) {
      if (rgb->r == value) {
        return;
      }
      rgb->r = value;
    } else if (channel == 1) {
      if (rgb->g == value) {
        return;
      }
      rgb->g = value;
    } else if (channel == 2) {
      if (rgb->b == value) {
        return;
      }
      rgb->b = value;
    }
    this->dirty = true;
}


void FastLEDChannelManager::onApply(int frame) {
    if (!this->dirty) {
        return;
    }
    // Serial.printf("FastLEDChannelManager::onApply for #%d\n", frame);
    for (int i = 0; i < this->num_leds; i ++) {
        rgb_t* rgb = &this->rgbs[i];
        this->leds[i].setRGB((int8_t)rgb->r, (int8_t)rgb->g, (int8_t)rgb->b);
    }
    FastLED.show();
    this->dirty = false;
}

FastLEDChannelSink::FastLEDChannelSink(uint8_t sink, FastLEDChannelManager* manager, int led, int channel): ChannelSink(sink) {
    this->manager = manager;
    this->led = led;
    this->channel = channel;
}

FastLEDChannelSink::~FastLEDChannelSink() {
}

void FastLEDChannelSink::setValue(int frame, float value) {
    // Serial.printf("FastLEDChannel(%d)::Channel(%d)::setValue(%f) for #%d\n", this->led, this->channel, value, frame);
    this->manager->setLeds(this->led, this->channel, value);
}

void FastLEDChannelSink::apply(int frame) {
    ;
}

#endif
