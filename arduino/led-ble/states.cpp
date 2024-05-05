#include "states.h"

Animation::Animation(unsigned char from_state, unsigned char to_state) {
    this->from_state = from_state;
    this->to_state = to_state;
    this->channels = new ChannelCollection();
}

Animation::~Animation() {
    if (this->channels != NULL) {
        delete this->channels;
        this->channels = NULL;
    }
}

bool Animation::isTarget(unsigned char from_state, unsigned char to_state) {
    return this->from_state == from_state && this->to_state == to_state;
}

ChannelCollection* Animation::getChannels() {
    return this->channels;
}

bool Animation::update(int frame) {
    return this->channels->update(frame);
}

size_t Animation::writeTo(void *buffer, size_t size) {
    animation_header_t header = {this->from_state, this->to_state};
    size_t header_size = sizeof(animation_header_t);
    size_t total_size = header_size + this->channels->writeTo(NULL, 0);
    if (buffer == NULL) {
        return total_size;
    }
    if (size < total_size) {
        Serial.printf("Animation::writeTo: buffer size is too small: %d < %d\n", size, total_size);
        return 0;
    }
    memcpy(buffer, &header, header_size);
    this->channels->writeTo((char *)buffer + header_size, size - header_size);
    return total_size;
}

Animation* Animation::readFrom(const void *buffer, size_t size, size_t* sizeRead) {
    if (size < sizeof(animation_header_t)) {
        Serial.printf("Animation::readFrom: buffer size is too small: %d < %d\n", size, sizeof(animation_header_t));
        return NULL;
    }
    animation_header_t *header = (animation_header_t *)buffer;
    size_t offset = sizeof(animation_header_t);
    size_t channelRead = 0;
    ChannelCollection *channels = ChannelCollection::readFrom((char *)buffer + offset, size - offset, &channelRead);
    if (channels == NULL) {
        return NULL;
    }
    if (sizeRead != NULL) {
        *sizeRead = offset + channelRead;
    }
    Animation *animation = new Animation(header->from_state, header->to_state);
    ChannelCollection* oldChannels = animation->channels;
    animation->channels = channels;
    delete oldChannels;
    oldChannels = NULL;
    return animation;
}

size_t Animation::writeHeaderTo(void *buffer, size_t size) {
    animation_header_t header = {this->from_state, this->to_state};
    size_t header_size = sizeof(animation_header_t);
    if (buffer == NULL) {
        return header_size;
    }
    if (size < header_size) {
        Serial.printf("Animation::writeHeaderTo: buffer size is too small: %d < %d\n", size, header_size);
        return 0;
    }
    memcpy(buffer, &header, header_size);
    return header_size;
}

States::States(unsigned char initial_state, int delay_msec, int buffer_size) {
    this->buffer_size_unit = buffer_size;
    this->buffer_size = buffer_size;
    this->num_animations = 0;
    this->animations = (Animation **)malloc(sizeof(Animation *) * buffer_size);
    this->initial_state = initial_state;
    this->delay_msec = delay_msec;
    this->current_state = initial_state;
    this->next_state = -1;
    this->current_frames = 0;
    this->num_next_states = 0;
    this->current_animation = NULL;
}

States::~States() {
    if (this->animations == NULL) {
        return;
    }
    for (int i = 0; i < this->num_animations; i++) {
        delete this->animations[i];
    }
    free(this->animations);
    this->animations = NULL;
}

bool States::reserveState(unsigned char state) {
    if (this->num_next_states >= MAX_NEXT_STATES) {
        return false;
    }
    this->next_states[this->num_next_states] = state;
    this->num_next_states++;
    return true;
}

void States::add(Animation *animation) {
    if (this->num_animations >= this->buffer_size) {
        // extend buffer
        this->buffer_size += this->buffer_size_unit;
        Animation** old_buffer = this->animations;
        this->animations = (Animation **)malloc(sizeof(Animation *) * this->buffer_size);
        memcpy(this->animations, old_buffer, sizeof(Animation *) * this->num_animations);
        free(old_buffer);
        old_buffer = NULL;
    }
    this->animations[this->num_animations] = animation;
    this->num_animations++;
}

void States::replace(Animation *animation) {
    for (int i = 0; i < this->num_animations; i++) {
        if (this->animations[i]->isTarget(animation->from_state, animation->to_state)) {
            delete this->animations[i];
            this->animations[i] = animation;
            return;
        }
    }
    this->add(animation);
}

void States::loop() {
    if (this->current_animation == NULL) {
        // decide next animation
        this->next_state = this->popNextState();
        this->current_animation = this->getAnimation(this->current_state, this->next_state);
        if (this->current_animation != NULL) {
            Serial.printf("start animation: %d -> %d\n", this->current_state, this->next_state);
        }
        this->current_frames = 0;
    }
    if (this->progressAnimation()) {
        delay(this->delay_msec);
        return;
    }
    // transition to next state
    this->current_state = this->next_state;
}

size_t States::writeTo(void *buffer, size_t size) {
    states_header_t header = {this->delay_msec, this->initial_state, this->num_animations};
    size_t header_size = sizeof(states_header_t);
    size_t total_size = header_size;
    for (int i = 0; i < this->num_animations; i++) {
        total_size += this->animations[i]->writeTo(NULL, 0);
    }
    if (buffer == NULL) {
        return total_size;
    }
    if (size < total_size) {
        Serial.printf("States::writeTo: buffer size is too small: %d < %d\n", size, total_size);
        return 0;
    }
    memcpy(buffer, &header, header_size);
    size_t offset = header_size;
    for (int i = 0; i < this->num_animations; i++) {
        offset += this->animations[i]->writeTo((char *)buffer + offset, size - offset);
    }
    return total_size;
}

size_t States::writeHeaderTo(void *buffer, size_t size) {
    states_header_t header = {this->delay_msec, this->initial_state, this->num_animations};
    size_t header_size = sizeof(states_header_t);
    size_t total_size = header_size;
    for (int i = 0; i < this->num_animations; i++) {
        total_size += this->animations[i]->writeHeaderTo(NULL, 0);
    }
    if (buffer == NULL) {
        return total_size;
    }
    if (size < total_size) {
        Serial.printf("States::writeHeaderTo: buffer size is too small: %d < %d\n", size, total_size);
        return 0;
    }
    memcpy(buffer, &header, header_size);
    size_t offset = header_size;
    for (int i = 0; i < this->num_animations; i++) {
        offset += this->animations[i]->writeHeaderTo((char *)buffer + offset, size - offset);
    }
    return total_size;
}

States* States::readFrom(const void *buffer, size_t size) {
    if (size < sizeof(states_header_t)) {
        Serial.printf("States::readFrom: buffer size is too small: %d < %d\n", size, sizeof(states_header_t));
        return NULL;
    }
    states_header_t *header = (states_header_t *)buffer;
    size_t offset = sizeof(states_header_t);
    States *states = new States(header->initial_state, header->delay_msec);
    for (int i = 0; i < header->num_animations; i++) {
        size_t animationRead = 0;
        Animation *animation = Animation::readFrom((char *)buffer + offset, size - offset, &animationRead);
        if (animation == NULL) {
            delete states;
            return NULL;
        }
        states->add(animation);
        offset += animationRead;
    }
    return states;
}

unsigned char States::popNextState() {
    if (this->num_next_states == 0) {
        return this->current_state;
    }
    this->num_next_states --;
    int next_state = this->next_states[0];
    for (int i = 0; i < this->num_next_states; i++) {
        this->next_states[i] = this->next_states[i + 1];
    }
    return next_state;
}

bool States::progressAnimation() {
    if (this->current_animation == NULL) {
        return false;
    }
    if (this->current_animation->update(this->current_frames)) {
        this->current_frames++;
        return true;
    }
    this->current_animation = NULL;
    this->current_frames = 0;
    return false;
}

Animation* States::getAnimation(int from_state, int to_state) {
    for (int i = 0; i < this->num_animations; i++) {
        if (this->animations[i]->isTarget(from_state, to_state)) {
            return this->animations[i];
        }
    }
    return NULL;
}
