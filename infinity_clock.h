#include "esphome.h"

#define INF_CLOCK_TAG "infinityclk"
#define INF_CLOCK_EFFECT_NAME "Infinity Clock"

class InfinityClock: public Component {
    static const esphome::Color color_cardinal_directions;
    static const esphome::Color color_proximity_makers;
    static const uint8_t proximity_markers_distance = 3;
    static const esphome::Color color_hour_hand;
    static const esphome::Color color_minute_hand;
    static const esphome::Color color_second_hand;

    static const uint16_t minutes_animation_before_ms = 1500;
    static const uint16_t seconds_animation_before_ms = 200;

    class Effect: public ::AddressableLightEffect {
        public:
            explicit Effect(): AddressableLightEffect(INF_CLOCK_EFFECT_NAME) {
                hour = minute = second = 0;
            }
            virtual ~Effect() {}

            virtual void apply(AddressableLight &it, const esphome::Color &current_color) override {
                // clear all LED's
                it.all() = esphome::Color::BLACK;

                if (hasTime) {
                    // map hour (12h) to LED index
                    const uint8_t _hour = (hour % 12) * 5;
                    it[map(_hour - 1)] = it[map(_hour - 1)].get() + color_hour_hand;
                    it[map(_hour)] = it[map(_hour)].get() + color_hour_hand;
                    it[map(_hour + 1)] = it[map(_hour + 1)].get() + color_hour_hand;

                    const auto ms = calcMillisClamped();
                    if (second * 1000 + ms < 60000 - minutes_animation_before_ms) {
                        it[map(minute)] = it[map(minute)].get() + color_minute_hand;
                    } else {
                        const uint8_t scale = 255 * (60000 - (second * 1000 + ms)) / minutes_animation_before_ms;
                        it[map(minute)] = it[map(minute)].get() + color_minute_hand * scale;
                        it[map(minute + 1)] = it[map(minute + 1)].get() + color_minute_hand * (255 - scale);
                    }

                    if (ms < 1000 - seconds_animation_before_ms) {
                        it[map(second)] = it[map(second)].get() + color_second_hand;
                    } else {
                        const uint8_t scale = 255 * (1000 - ms) / seconds_animation_before_ms;
                        it[map(second)] = it[map(second)].get() + color_second_hand * scale;
                        it[map(second + 1)] = it[map(second + 1)].get() + color_second_hand * (255 - scale);
                    }

                    // set markers for N/E/S/W directions
                    set_if_clear(it, 0, color_cardinal_directions);
                    set_if_clear(it, 15, color_cardinal_directions);
                    set_if_clear(it, 30, color_cardinal_directions);
                    set_if_clear(it, 45, color_cardinal_directions);

                    // set lighter markers for every 5 minutes when there's a clock hand nearby
                    set_if_clear(it, 5, color_proximity_makers, proximity_markers_distance);
                    set_if_clear(it, 10, color_proximity_makers, proximity_markers_distance);
                    set_if_clear(it, 20, color_proximity_makers, proximity_markers_distance);
                    set_if_clear(it, 25, color_proximity_makers, proximity_markers_distance);
                    set_if_clear(it, 35, color_proximity_makers, proximity_markers_distance);
                    set_if_clear(it, 40, color_proximity_makers, proximity_markers_distance);
                    set_if_clear(it, 55, color_proximity_makers, proximity_markers_distance);
                } else {
                    // spinning loading animation
                    it[59 - ((millis() / 20) % 60)] = esphome::Color::WHITE;
                }

                it.schedule_show();
            }

            void set_time(uint8_t hour, uint8_t minute, uint8_t second) {
                if (this->second != second) {
                    millis0 = millis();
                }

                this->hour = hour;
                this->minute = minute;
                this->second = second;

                this->hasTime = true;
            }
        private:
            static const int8_t led_offset = 29;
            uint8_t hour, minute, second;
            /// millis() when SNTP time millis was around 0 and the second incremented, required for animations
            uint32_t millis0 {0};
            bool hasTime {false};

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

            /**
             * sets LED to color if it is not on
             * @param led_index index relative to 0 at the top of the clock, might be negative
             * @param color color to set on LED if not on
             * @param proximity also check whether any LED is on next to led_index in this proximity
             */
            void set_if_clear(AddressableLight &it, int8_t led_index, esphome::Color color, uint8_t proximity = 0) {
                if (it[map(led_index)].get().is_on()) {
                    return;  // the LED is already lit up, don't change it
                }

                if (proximity == 0) {
                    // proximity feature is not used, enable it
                    it[map(led_index)] = color;
                } else {
                    for (uint8_t i = 0; i <= proximity; i++) {
                        if (it[map(led_index - i)].get().is_on() || it[map(led_index + i)].get().is_on()) {
                            // found a LED nearby, enable it and return
                            it[map(led_index)] = color;
                            return;
                        }
                    }
                }
            }

            /**
             * estimates SNTP millis()
             *
             * @return uint16_t current time milliseconds [0-999]
             */
            uint16_t calcMillisClamped() {
                const uint32_t delta = millis() - millis0;
                if (delta > 999) {
                    return 999;
                }
                return delta;
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
        const ESPTime time = sntp_time->now();
        if (time.is_valid()) {
            effect->set_time(time.hour, time.minute, time.second);
        }
    }
private:
    Effect *effect;
};

const esphome::Color InfinityClock::color_cardinal_directions = esphome::Color(0x303030);
const esphome::Color InfinityClock::color_proximity_makers = esphome::Color(0x202020);
const esphome::Color InfinityClock::color_hour_hand = esphome::Color(0xff0000);
const esphome::Color InfinityClock::color_minute_hand = esphome::Color(0x00ff00);
const esphome::Color InfinityClock::color_second_hand = esphome::Color(0x0000ff);
