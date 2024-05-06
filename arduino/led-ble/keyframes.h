#pragma once
#include "Arduino.h"

#define MAX_CHANNELS 32
#define MAX_SINKS 32

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

typedef struct __channel_header_t {
    uint8_t sink;
    uint8_t buffer_size;
} channel_header_t;

class ChannelSink {
    public:
        uint8_t sink;
        ChannelSink(uint8_t sink);
        virtual void setValue(int frame, float value) = 0;
};

class AnalogWriteChannelSink : public ChannelSink {
    private:
        uint8_t pin;
    
    public:
        AnalogWriteChannelSink(uint8_t sink, uint8_t pin);
        ~AnalogWriteChannelSink();
        void setValue(int frame, float value);
};

class ChannelSinks {
    private:
        int num_sinks;
        ChannelSink* sinks[MAX_SINKS];
    
    public:
        ChannelSinks();
        ~ChannelSinks();
        ChannelSink* add(ChannelSink *sink);
        void setValue(uint8_t sink, int frame, float value);
};

class Channel {
    protected:
        Keyframes keyframes;
        unsigned char sink;

    public:
        static Channel* readFrom(const void *buffer, size_t size, size_t* sizeRead);
        Channel(unsigned char sink, int buffer_size=16);
        void add(const keyframe_t &keyframe);
        void add(int frame, float value);
        bool update(ChannelSinks* sinks, int frame);
        size_t writeTo(void *buffer, size_t size);
};

typedef struct __channel_collection_header_t {
    uint8_t num_channels;
} channel_collection_header_t;

class ChannelCollection {
    private:
        unsigned char num_channels;
        Channel* channels[MAX_CHANNELS];
    
    public:
        static ChannelCollection* readFrom(const void *buffer, size_t size, size_t* sizeRead);
        ChannelCollection();
        ~ChannelCollection();
        Channel* add(Channel *channel);
        bool update(ChannelSinks* sinks, int frame);
        size_t writeTo(void *buffer, size_t size);
};
