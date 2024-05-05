#pragma once
#include "Arduino.h"

#define MAX_CHANNELS 16

#define SIZE_KEYFRAME_BYTES 8

#define KEYFRAME_TYPE_PWM 1

typedef struct __keyframe_t {
    uint8_t type;
    uint16_t frame;
    float value;
} keyframe_t;

class Keyframes {
    private:
        int buffer_size_unit;
        int buffer_size;
  
    public:
        int num_keyframes;
        keyframe_t *keyframes;
    
        static Keyframes* readFrom(const void *buffer, size_t size);
        Keyframes(int buffer_size=16);
        ~Keyframes();
        void add(const keyframe_t &keyframe);
        void add(int frame, float value);
        float getValue(int frame);
        int getLastFrame();
        size_t writeTo(void *buffer, size_t size);
};

#define CHANNEL_TYPE_LED 1

typedef struct __base_channel_header_t {
    uint8_t type;
    uint8_t buffer_size;
} base_channel_header_t;

class BaseChannel {
    protected:
        Keyframes keyframes;
    
    public:
        static BaseChannel* readFrom(const void *buffer, size_t size, size_t* sizeRead);
        BaseChannel(int buffer_size=16);
        void add(const keyframe_t &keyframe);
        void add(int frame, float value);
        bool update(int frame);
        virtual size_t writeTo(void *buffer, size_t size) = 0;

    protected:
        virtual void setValue(int frame, float value) = 0;
};

typedef struct __led_channel_header_t {
    base_channel_header_t base;
    uint8_t pin;
} led_channel_header_t;

class LEDChannel : public BaseChannel {
    private:
        unsigned char pin;
    
    public:
        LEDChannel(unsigned char pin, int buffer_size=16);
        size_t writeTo(void *buffer, size_t size);
    
    protected:
        void setValue(int frame, float value);
};

typedef struct __channel_collection_header_t {
    uint8_t num_channels;
} channel_collection_header_t;

class ChannelCollection {
    private:
        unsigned char num_channels;
        BaseChannel* channels[MAX_CHANNELS];
    
    public:
        static ChannelCollection* readFrom(const void *buffer, size_t size, size_t* sizeRead);
        ChannelCollection();
        ~ChannelCollection();
        BaseChannel* add(BaseChannel *channel);
        bool update(int frame);
        size_t writeTo(void *buffer, size_t size);
};
