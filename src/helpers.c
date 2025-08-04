#include "./config.h"
#include "./helpers.h"


void helper_draw_long_press(u8g2_t *u8g2, AppInput const *input) {
    if (input->pressed_during_ms[BTN_A] > PUSH_BTN_REPEAT_MS) {
        uint8_t value = input->pressed_during_ms[BTN_A] * 100
            / PUSH_BTN_LONG_PRESS_MS;
        if (value > 100) {
            value = 100;
        }
        u8g2_DrawFrame(u8g2, 12, 58, 104, 6);
        u8g2_DrawBox(u8g2, 14, 60, value, 2);
    }
};
