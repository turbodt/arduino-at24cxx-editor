#include "./start.h"
#include "../config.h"
#include "../helpers.h"
#include "../app-input.h"
#include <u8g2.h>
#include <Arduino.h>
#include <stdio.h>


typedef struct {
    uint8_t should_exit;
    uint8_t pressed_during_pc;
} ScreenState;
static void app_draw(AppState *app_state, ScreenState *screen_state);


void screen_start(AppState *app_state) {
    ScreenState screen_state = {
        .should_exit = 0,
        .pressed_during_pc = 0,
    };
    while (!screen_state.should_exit) {
        app_input_read(&app_state->input);

        if (smart_button_has_falled(app_state->input.push_btns[BTN_A])) {
            screen_state.pressed_during_pc = 0;
        } else if (
            app_state->input.pressed_during_ms[BTN_A] > PUSH_BTN_LONG_PRESS_MS
        ) {
            screen_state.should_exit = 1;
            continue;
        } else {
            screen_state.pressed_during_pc =
                app_state->input.pressed_during_ms[BTN_A] * 100
                / PUSH_BTN_LONG_PRESS_MS;
        }
        app_draw(app_state, &screen_state);
    }

    screen_state.pressed_during_pc = 100;
    screen_state.should_exit = 0;
    while (!screen_state.should_exit) {
        app_input_read(&app_state->input);

        if (smart_button_has_falled(app_state->input.push_btns[BTN_A])) {
            screen_state.should_exit = 1;
        }
        app_draw(app_state, &screen_state);
    }

    app_input_wait_for_total_release(&app_state->input, NULL, NULL);
};



void app_draw(AppState *app_state, ScreenState *screen_state) {
    u8g2_t *const u8g2 = (u8g2_t *)app_state->display;
    uint8_t animation_offset = 0;
    if (screen_state->pressed_during_pc == 0) {
        uint8_t value = (millis() % 1000) * 200 / 1000;
        if (value > 100) {
            value = 200 -value;
        }

        if (value < 10) {
            animation_offset = 0;
        } else if (value < 30) {
            animation_offset = 1;
        } else if (value < 60) {
            animation_offset = 2;
        } else if (value < 90) {
            animation_offset = 3;
        } else {
            animation_offset = 4;
        }
    }

    u8g2_FirstPage(u8g2);
    do {
        u8g2_ClearBuffer(u8g2);

        if (screen_state->pressed_during_pc < 100) {
            u8g2_DrawStr(u8g2, 16, 16, "Connect the");
            u8g2_DrawStr(u8g2, 20, 30, "device and");
            u8g2_DrawStr(u8g2, 4, 50 + animation_offset, "Hold A to start!");
        } else {
            u8g2_DrawStr(u8g2, 20, 32+4, "Release");
        }

        helper_draw_long_press(u8g2, &app_state->input);

        u8g2_SendBuffer(u8g2);
    } while (u8g2_NextPage(u8g2));
}
