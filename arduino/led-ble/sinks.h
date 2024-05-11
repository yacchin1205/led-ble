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

class FastLEDChannelManager : public IChannelSinkApplyListener {
    private:
        CRGB* leds;
        int num_leds;
    
    public:
        FastLEDChannelManager(int num_leds);
        ~FastLEDChannelManager();
        CRGB* getLeds();
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
