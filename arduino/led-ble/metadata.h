#pragma once
#include "Arduino.h"

#define SIZE_UUID 16
#define MAX_SIZE_DEVICE_NAME 64
#define MAX_SIZE_CHANNEL_NAME 64

typedef struct __channel_metdata_t {
    uint8_t type;
    uint8_t led_pin;

    uint8_t channel_name_size;
    char* channel_name;
} channel_metdata_t;

typedef struct __metadata_t {
    uint8_t uuid[SIZE_UUID];

    uint8_t device_name_size;
    char* device_name;

    uint8_t num_channels;
    channel_metdata_t* channels;
} metadata_t;

class ChannelMetadata {
    private:
        uint8_t type;
        uint8_t led_pin;
        char channel_name[MAX_SIZE_DEVICE_NAME];

    public:
        ChannelMetadata(uint8_t type, uint8_t led_pin, const char* channel_name);
        ~ChannelMetadata();
        size_t writeTo(void *buffer, size_t size);
};

class Metadata {
    private:
        char device_name[MAX_SIZE_DEVICE_NAME];
        uint8_t uuid[SIZE_UUID];
        int buffer_size_unit;
        int buffer_size;
        int num_channels;
        ChannelMetadata** channels;

    public:
        Metadata(const char* device_name, const uint8_t* uuid, int buffer_size=16);
        ~Metadata();
        ChannelMetadata* add(uint8_t type, uint8_t led_pin, const char* channel_name);
        size_t writeTo(void *buffer, size_t size);
};
