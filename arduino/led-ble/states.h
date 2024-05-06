#pragma once
#include "keyframes.h"

#define MAX_NEXT_STATES 16

typedef struct __animation_header_t {
    uint8_t from_state;
    uint8_t to_state;
} animation_header_t;

class Animation {
    public:
        unsigned char from_state;
        unsigned char to_state;

    private:
        ChannelCollection* channels;

    public:
        static Animation* readFrom(const void *buffer, size_t size, size_t* sizeRead);
        Animation(unsigned char from_state, unsigned char to_state);
        ~Animation();
        bool isTarget(unsigned char from_state, unsigned char to_state);
        ChannelCollection* getChannels();
        bool update(ChannelSinks* sinks, int frame);
        size_t writeTo(void *buffer, size_t size);
        size_t writeHeaderTo(void *buffer, size_t size);
};

typedef struct __states_header_t {
    uint16_t delay_msec;
    uint8_t initial_state;
    uint8_t num_animations;
} states_header_t;

class States {
    private:
        int num_animations;
        int buffer_size_unit;
        int buffer_size;
        Animation **animations;
        unsigned char initial_state;
        int delay_msec;
        unsigned char current_state;
        unsigned char next_state;
        int current_frames;
        int num_next_states;
        unsigned char next_states[MAX_NEXT_STATES];
        Animation *current_animation;

    public:
        static States* readFrom(const void *buffer, size_t size);
        States(unsigned char initial_state, int delay_msec=100, int buffer_size=16);
        ~States();
        bool reserveState(unsigned char state);
        void add(Animation *animation);
        void replace(Animation *animation);
        void loop(ChannelSinks* sinks);
        size_t writeTo(void *buffer, size_t size);
        size_t writeHeaderTo(void *buffer, size_t size);
        unsigned char getAnimationCount() { return this->num_animations; }
        Animation* getAnimation(int index) { return this->animations[index]; }
        Animation* getAnimation(int from_state, int to_state);

    private:
        unsigned char popNextState();
        bool progressAnimation(ChannelSinks* sinks);
};
