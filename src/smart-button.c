#include "./smart-button.h"
#include <Arduino.h>

#ifndef DEBOUNCE_TIME_MS
#define DEBOUNCE_TIME_MS 50
#endif


struct SmartButton {
    uint8_t pin;
    uint8_t has_been_checked;
    int last_state;
    unsigned long last_change_at;
    unsigned long repeat_after_ms;
};
static uint8_t update_state_if_needed(SmartButton *);

SmartButton * smart_button_make(uint8_t pin, unsigned long repeat_after_ms) {
    SmartButton *impl = malloc(sizeof(SmartButton));
    if (impl == NULL) {
        return NULL;
    }

    pinMode(pin, INPUT);

    impl->pin = pin;
    impl->has_been_checked = 0;
    impl->last_state = digitalRead(pin);
    impl->last_change_at = millis();
    impl->repeat_after_ms = repeat_after_ms;

    return impl;
};


inline void smart_button_destroy(SmartButton *impl) {
    free(impl);
};

inline uint8_t smart_button_is_pressed(SmartButton *impl) {
    update_state_if_needed(impl);
    return impl->last_state;
};

inline uint8_t smart_button_has_raised(SmartButton *impl) {
    update_state_if_needed(impl);
    if (!impl->last_state) {
        return 0;
    }
    if (impl->has_been_checked) {
        return 0;
    }
    impl->has_been_checked = 1;
    return 1;
};

inline uint8_t smart_button_has_falled(SmartButton *impl) {
    update_state_if_needed(impl);
    if (impl->last_state) {
        return 0;
    }
    if (impl->has_been_checked) {
        return 0;
    }
    impl->has_been_checked = 1;
    return 1;
};

unsigned long int smart_button_pressed_during_ms(SmartButton *impl) {
    update_state_if_needed(impl);
    if (!impl->last_state) {
        return 0;
    }

    return millis() - impl->last_change_at;
};


inline uint8_t update_state_if_needed(SmartButton *impl) {
    int curr_state = digitalRead(impl->pin);
    if (curr_state == impl->last_state && !impl->last_state) {
        return 0;
    }

    unsigned long int t_diff_ms = millis() - impl->last_change_at;
    if (curr_state == impl->last_state) {
        if (impl->repeat_after_ms == 0 || t_diff_ms < impl->repeat_after_ms) {
            return 0;
        }
    } else if (DEBOUNCE_TIME_MS > t_diff_ms) {
        return 0;
    }

    impl->last_state = curr_state;
    impl->last_change_at = millis();
    impl->has_been_checked = 0;

    return 1;
};
