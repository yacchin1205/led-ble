#include "metadata.h"

ChannelMetadata::ChannelMetadata(uint8_t type, uint8_t sink, const char* channel_name) {
    this->type = type;
    this->sink = sink;
    strcpy(this->channel_name, channel_name);
}

ChannelMetadata::~ChannelMetadata() {
}

size_t ChannelMetadata::writeTo(void *buffer, size_t size) {
    if (buffer == NULL) {
        return sizeof(channel_metdata_t) + strlen(this->channel_name);
    }
    if (size < sizeof(channel_metdata_t) + strlen(this->channel_name)) {
        return 0;
    }
    channel_metdata_t metadata = {this->type, this->sink, strlen(this->channel_name)};
    memcpy(buffer, &metadata, sizeof(channel_metdata_t));
    memcpy((char*)buffer + sizeof(channel_metdata_t), this->channel_name, strlen(this->channel_name));
    return sizeof(channel_metdata_t) + strlen(this->channel_name);
}

Metadata::Metadata(const char* device_name, const uint8_t* uuid, int buffer_size) {
    strcpy(this->device_name, device_name);
    memcpy(this->uuid, uuid, SIZE_UUID);
    this->buffer_size_unit = buffer_size;
    this->buffer_size = buffer_size;
    this->num_channels = 0;
    this->channels = (ChannelMetadata**)malloc(buffer_size * sizeof(ChannelMetadata*));
}

Metadata::~Metadata() {
    if (this->channels != NULL) {
        for (int i = 0; i < this->num_channels; i++) {
            delete this->channels[i];
            this->channels[i] = NULL;
        }
        free(this->channels);
        this->channels = NULL;
    }
}

ChannelMetadata* Metadata::add(uint8_t type, uint8_t sink, const char* channel_name) {
    if (this->num_channels >= this->buffer_size) {
        // extend buffer
        this->buffer_size += this->buffer_size_unit;
        ChannelMetadata** old_buffer = this->channels;
        this->channels = (ChannelMetadata**)malloc(this->buffer_size * sizeof(ChannelMetadata*));
        memcpy(this->channels, old_buffer, this->num_channels * sizeof(ChannelMetadata*));
        free(old_buffer);
        old_buffer = NULL;
    }
    ChannelMetadata* channel = new ChannelMetadata(type, sink, channel_name);
    this->channels[this->num_channels] = channel;
    this->num_channels++;
    return channel;
}

size_t Metadata::writeTo(void *buffer, size_t size) {
    if (buffer == NULL) {
        size_t size = sizeof(metadata_t);
        size += strlen(this->device_name);
        for (int i = 0; i < this->num_channels; i++) {
            size += this->channels[i]->writeTo(NULL, 0);
        }
        return size;
    }
    if (size < sizeof(metadata_t)) {
        return 0;
    }
    metadata_t metadata;
    memcpy(metadata.uuid, this->uuid, SIZE_UUID);
    metadata.device_name_size = strlen(this->device_name);
    metadata.num_channels = this->num_channels;
    metadata.device_name = NULL;
    metadata.channels = NULL;
    memcpy(buffer, &metadata, sizeof(metadata_t));
    metadata.device_name = (char*)buffer + sizeof(metadata_t);
    memcpy(metadata.device_name, this->device_name, strlen(this->device_name));
    size_t offset = sizeof(metadata_t) + strlen(this->device_name);
    for (int i = 0; i < this->num_channels; i++) {
        size_t channel_size = this->channels[i]->writeTo((char*)buffer + offset, size - offset);
        if (channel_size == 0) {
            return 0;
        }
        offset += channel_size;
    }
    return offset;
}
