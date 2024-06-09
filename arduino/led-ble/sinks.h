#include "keyframes.h"

// Enable FastLED support
#define ENABLE_FASTLED

#ifdef ENABLE_FASTLED
#include <FastLED.h>
#endif

class AnalogWriteChannelSink : public ChannelSink {
    private:
        uint8_t pin;
    
    public:
        AnalogWriteChannelSink(uint8_t sink, uint8_t pin);
        ~AnalogWriteChannelSink();
        void setValue(int frame, float value);
        void apply(int frame);
};

#ifdef ENABLE_FASTLED

typedef struct __rgb_t {
  float r;
  float g;
  float b;
} rgb_t;

class FastLEDChannelManager : public IChannelSinkApplyListener {
    private:
        CRGB* leds;
        rgb_t* rgbs;
        int num_leds;
        bool dirty;
    
    public:
        FastLEDChannelManager(int num_leds);
        ~FastLEDChannelManager();
        CRGB* getLeds();
        void setLeds(int led, int channel, float value);
        void onApply(int frame);
};

class FastLEDChannelSink : public ChannelSink {
    private:
        FastLEDChannelManager* manager;
        int led;
        int channel;

    public:
        FastLEDChannelSink(uint8_t sink, FastLEDChannelManager* manager, int led, int channel);
        ~FastLEDChannelSink();
        void setValue(int frame, float value);
        void apply(int frame);
};

#endif
