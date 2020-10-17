#include "esphome.h"

#define INF_CLOCK_TAG "infinityclk"
#define INF_CLOCK_EFFECT_NAME "Infinity Clock"

class InfinityClock: public Component {
    static const uint32_t color_cardinal_directions = 0x303030;
    static const ESPColor color_hour_hand;
    static const ESPColor color_minute_hand;
    static const ESPColor color_second_hand;

    class Effect: public ::AddressableLightEffect {
        public:
            explicit Effect(): AddressableLightEffect(INF_CLOCK_EFFECT_NAME) {
                hour = minute = second = 0;
            }
            virtual ~Effect() {}

            virtual void apply(AddressableLight &it, const ESPColor &current_color) override {
                it.all() = ESPColor::BLACK;

                it[map(0)] = color_cardinal_directions;
                it[map(15)] = color_cardinal_directions;
                it[map(30)] = color_cardinal_directions;
                it[map(45)] = color_cardinal_directions;

                // map hour (12h) to LED index
                const uint8_t _hour = (hour % 12) * 5;
                it[map(_hour - 1)] = it[map(_hour - 1)].get() + color_hour_hand;
                it[map(_hour)] = it[map(_hour)].get() + color_hour_hand;
                it[map(_hour + 1)] = it[map(_hour + 1)].get() + color_hour_hand;

                it[map(minute)] = it[map(minute)].get() + color_minute_hand;
                it[map(second)] = it[map(second)].get() + color_second_hand;
            }

            void set_time(uint8_t hour, uint8_t minute, uint8_t second) {
                this->hour = hour;
                this->minute = minute;
                this->second = second;
            }
        private:
            static const int8_t led_offset = 29;
            uint8_t hour, minute, second;

            /**
             * map minute/second counters to LED indices on the strip
             * @param led_index index relative to 0 at the top of the clock, might be negative
             * @return [0-59]
             */
            uint8_t map(int8_t led_index) {
                led_index = led_offset - led_index;
                if (led_index < 0) {
                    led_index += 60;
                }
                return led_index;
            }
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

const ESPColor InfinityClock::color_hour_hand = ESPColor(0xff0000);
const ESPColor InfinityClock::color_minute_hand = ESPColor(0x00ff00);
const ESPColor InfinityClock::color_second_hand = ESPColor(0x0000ff);
