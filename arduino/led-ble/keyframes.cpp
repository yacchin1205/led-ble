#include "keyframes.h"

Keyframes::Keyframes(int buffer_size) {
    this->buffer_size_unit = buffer_size;
    this->buffer_size = buffer_size;
    this->num_keyframes = 0;
    this->keyframes = (keyframe_t *)malloc(sizeof(keyframe_t) * buffer_size);
}

Keyframes::~Keyframes() {
    if (this->keyframes == NULL) {
        return;
    }
    free(this->keyframes);
    this->keyframes = NULL;
}

void Keyframes::add(const keyframe_t &keyframe) {
    if (this->num_keyframes >= this->buffer_size) {
        // extend buffer
        this->buffer_size += this->buffer_size_unit;
        keyframe_t* old_buffer = this->keyframes;
        this->keyframes = (keyframe_t *)malloc(sizeof(keyframe_t) * this->buffer_size);
        memcpy(this->keyframes, old_buffer, sizeof(keyframe_t) * this->num_keyframes);
        free(old_buffer);
        old_buffer = NULL;
    }
    this->keyframes[this->num_keyframes] = keyframe;
    this->num_keyframes++;
}

// Float values for PWM
void Keyframes::add(int frame, float value) {
    keyframe_t keyframe = {KEYFRAME_TYPE_PWM, frame, value};
    this->add(keyframe);
}

float Keyframes::getValue(int frame) {
    if (this->num_keyframes == 0) {
        return 0;
    }
    if (frame < this->keyframes[0].frame) {
        return this->keyframes[0].value;
    }
    for (int i = 0; i < this->num_keyframes - 1; i++) {
        if (frame >= this->keyframes[i].frame && frame < this->keyframes[i + 1].frame) {
            float t = (float)(frame - this->keyframes[i].frame) / (this->keyframes[i + 1].frame - this->keyframes[i].frame);
            return this->keyframes[i].value + t * (this->keyframes[i + 1].value - this->keyframes[i].value);
        }
    }
    return this->keyframes[this->num_keyframes - 1].value;
}

int Keyframes::getLastFrame() {
    if (this->num_keyframes == 0) {
        return 0;
    }
    return this->keyframes[this->num_keyframes - 1].frame;
}

size_t Keyframes::writeTo(void *buffer, size_t size) {
    if (sizeof(keyframe_t) != SIZE_KEYFRAME_BYTES) {
        Serial.printf("Keyframes::writeTo: invalid keyframe size: %d != %d\n", sizeof(keyframe_t), SIZE_KEYFRAME_BYTES);
        return 0;
    }
    if (buffer == NULL) {
        return sizeof(keyframe_t) * this->num_keyframes;
    }
    if (size < sizeof(keyframe_t) * this->num_keyframes) {
        Serial.printf("Keyframes::writeTo: buffer size is too small: %d < %d\n", size, sizeof(keyframe_t) * this->num_keyframes);
        return 0;
    }
    memcpy(buffer, this->keyframes, sizeof(keyframe_t) * this->num_keyframes);
    return sizeof(keyframe_t) * this->num_keyframes;
}

Keyframes* Keyframes::readFrom(const void *buffer, size_t size) {
    if (sizeof(keyframe_t) != SIZE_KEYFRAME_BYTES) {
        Serial.printf("Keyframes::writeTo: invalid keyframe size: %d != %d\n", sizeof(keyframe_t), SIZE_KEYFRAME_BYTES);
        return 0;
    }
    if (size % sizeof(keyframe_t) != 0) {
        Serial.printf("Keyframes::readFrom: invalid buffer size: %d\n", size);
        return NULL;
    }
    Keyframes *keyframes = new Keyframes(size / sizeof(keyframe_t));
    const keyframe_t *keyframe = (const keyframe_t *)buffer;
    for (int i = 0; i < size / sizeof(keyframe_t); i++) {
        keyframes->add(keyframe[i]);
    }
    return keyframes;
}

BaseChannel::BaseChannel(int buffer_size): keyframes(buffer_size) {
}

void BaseChannel::add(const keyframe_t &keyframe) {
    this->keyframes.add(keyframe);
}

void BaseChannel::add(int frame, float value) {
    this->keyframes.add(frame, value);
}

bool BaseChannel::update(int frame) {
    if (frame > this->keyframes.getLastFrame()) {
        return false;
    }
    this->setValue(frame, this->keyframes.getValue(frame));
    return true;
}

BaseChannel* BaseChannel::readFrom(const void *buffer, size_t size, size_t* sizeRead) {
    if (size < sizeof(base_channel_header_t)) {
        Serial.printf("BaseChannel::readFrom: invalid buffer size: %d\n", size);
        return NULL;
    }
    const base_channel_header_t *header = (const base_channel_header_t *)buffer;
    if (header->type == CHANNEL_TYPE_LED) {
        if (size < sizeof(led_channel_header_t)) {
            Serial.printf("BaseChannel::readFrom: invalid buffer size: %d\n", size);
            return NULL;
        }
        const led_channel_header_t *led_header = (const led_channel_header_t *)buffer;
        if (size < sizeof(led_channel_header_t) + led_header->base.buffer_size) {
            Serial.printf("BaseChannel::readFrom: invalid buffer size: %d\n", size);
            return NULL;
        }
        Keyframes *keyframes = Keyframes::readFrom(led_header + 1,  led_header->base.buffer_size);
        if (keyframes == NULL) {
            Serial.println("BaseChannel::readFrom: failed to read keyframes");
            return NULL;
        }
        LEDChannel *channel = new LEDChannel(led_header->pin, keyframes->num_keyframes);
        for (int i = 0; i < keyframes->num_keyframes; i++) {
            channel->add(keyframes->keyframes[i]);
        }
        delete keyframes;
        keyframes = NULL;
        if (sizeRead != NULL) {
            *sizeRead = sizeof(led_channel_header_t) + led_header->base.buffer_size;
        }
        return channel;
    }
    Serial.printf("BaseChannel::readFrom: invalid channel type: %d\n", header->type);
    return NULL;
}

LEDChannel::LEDChannel(unsigned char pin, int buffer_size) : BaseChannel(buffer_size) {
    this->pin = pin;
    pinMode(this->pin, OUTPUT);
}

void LEDChannel::setValue(int frame, float value) {
    //Serial.printf("LEDChannel(%d)::setValue(%f) for #%d\n", this->pin, value, frame);
    analogWrite(this->pin, value);
}

size_t LEDChannel::writeTo(void *buffer, size_t size) {
    size_t keyframes_size = this->keyframes.writeTo(NULL, 0);
    led_channel_header_t header = {{CHANNEL_TYPE_LED, keyframes_size}, this->pin};
    size_t header_size = sizeof(led_channel_header_t);
    size_t total_size = header_size + keyframes_size;
    if (buffer == NULL) {
        return total_size;
    }
    if (size < total_size) {
        Serial.printf("LEDChannel::writeTo: buffer size is too small: %d < %d\n", size, total_size);
        return 0;
    }
    led_channel_header_t *header_ptr = (led_channel_header_t *)buffer;
    memcpy(header_ptr, &header, header_size);
    this->keyframes.writeTo((keyframe_t *)(header_ptr + 1), keyframes_size);
    return total_size;
}

ChannelCollection::ChannelCollection() {
    this->num_channels = 0;
    for (int i = 0; i < MAX_CHANNELS; i++) {
        this->channels[i] = NULL;
    }
}

ChannelCollection::~ChannelCollection() {
    for (int i = 0; i < this->num_channels; i++) {
        if (this->channels[i] == NULL) {
            continue;
        }
        delete this->channels[i];
        this->channels[i] = NULL;
    }
}

BaseChannel* ChannelCollection::add(BaseChannel *channel) {
    if (this->num_channels >= MAX_CHANNELS) {
        return NULL;
    }
    this->channels[this->num_channels] = channel;
    this->num_channels++;
    return channel;
}

bool ChannelCollection::update(int frame) {
    bool updated = false;
    for (int i = 0; i < this->num_channels; i++) {
        updated |= this->channels[i]->update(frame);
    }
    return updated;
}

size_t ChannelCollection::writeTo(void *buffer, size_t size) {
    channel_collection_header_t header = {this->num_channels};
    size_t header_size = sizeof(channel_collection_header_t);
    size_t total_size = header_size;
    if (buffer == NULL) {
        for (int i = 0; i < this->num_channels; i++) {
            total_size += this->channels[i]->writeTo(NULL, 0);
        }
        return total_size;
    }
    if (size < total_size) {
        Serial.printf("ChannelCollection::writeTo: buffer size is too small: %d < %d\n", size, total_size);
        return 0;
    }
    channel_collection_header_t *header_ptr = (channel_collection_header_t *)buffer;
    memcpy(header_ptr, &header, header_size);
    size_t offset = header_size;
    for (int i = 0; i < this->num_channels; i++) {
        size_t channel_size = this->channels[i]->writeTo((char *)buffer + offset, size - offset);
        offset += channel_size;
    }
    return total_size;
}

ChannelCollection* ChannelCollection::readFrom(const void *buffer, size_t size, size_t* sizeRead) {
    if (size < sizeof(channel_collection_header_t)) {
        Serial.printf("ChannelCollection::readFrom: invalid buffer size: %d\n", size);
        return NULL;
    }
    const channel_collection_header_t *header = (const channel_collection_header_t *)buffer;
    ChannelCollection *channels = new ChannelCollection();
    size_t offset = sizeof(channel_collection_header_t);
    for (int i = 0; i < header->num_channels; i++) {
        size_t channelRead = 0;
        BaseChannel *channel = BaseChannel::readFrom((char *)buffer + offset, size - offset, &channelRead);
        if (channel == NULL) {
            Serial.println("ChannelCollection::readFrom: failed to read channel");
            delete channels;
            return NULL;
        }
        offset += channelRead;
        channels->add(channel);
    }
    if (sizeRead != NULL) {
        *sizeRead = offset;
    }
    return channels;
}
