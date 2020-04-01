#include "esphome.h"

#define INF_CLOCK_TAG "infinityclk"
#define INF_CLOCK_EFFECT_NAME "Infinity Clock"

class InfinityClock: public Component {
    class Effect: public ::AddressableLightEffect {
        public:
            explicit Effect(): AddressableLightEffect(INF_CLOCK_EFFECT_NAME) {
                hour = minute = second = 0;
            }
            virtual ~Effect() {}

            virtual void apply(AddressableLight &it, const ESPColor &current_color) override {
                it.all() = ESPColor::BLACK;

                // map hour (12h) to LED index
                const uint8_t _hour = (hour % 12) * 5;
                it[_hour].set_red(255);
                it[minute].set_green(255);
                it[second].set_blue(255);
            }

            void set_time(uint8_t hour, uint8_t minute, uint8_t second) {
                this->hour = hour;
                this->minute = minute;
                this->second = second;
            }
        private:
            uint8_t hour, minute, second;
    };

public:
    virtual ~InfinityClock() {}

    virtual void setup() override {
        // register own custom effect on ledstrip
        effect = new Effect();
        effect->init_internal(ledstrip);
        ledstrip->add_effects({effect});

        // activate own effect
        auto call = ledstrip->make_call();
        call.set_state(true);
        call.set_effect(effect->get_name());
        call.perform();
    }

    virtual void loop() override {
        const ESPTime now = ha_time->now();
        effect->set_time(now.hour, now.minute, now.second);
    }
private:
    Effect *effect;
};
