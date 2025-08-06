#include "./config.h"
#include "./helpers.h"


void helper_draw_long_press(u8g2_t *u8g2, AppInput const *input) {
    if (input->pressed_during_ms[BTN_A] > PUSH_BTN_REPEAT_MS) {
        uint8_t value = input->pressed_during_ms[BTN_A] * 124
            / PUSH_BTN_LONG_PRESS_MS;
        if (value > 124) {
            value = 124;
        }
        u8g2_DrawFrame(u8g2, 0, 58, 128, 6);
        u8g2_DrawBox(u8g2, 2, 60, value, 2);
    } else if (input->pressed_during_ms[BTN_B] > PUSH_BTN_REPEAT_MS) {
        uint8_t value = input->pressed_during_ms[BTN_B] * 124
            / PUSH_BTN_LONG_PRESS_MS;
        if (value > 124) {
            value = 124;
        }
        u8g2_DrawFrame(u8g2, 0, 58, 128, 6);
        u8g2_DrawBox(u8g2, 126 - value, 60, value, 2);
    }
};
