#include "../config.h"
#include "./selector.h"
#include <u8g2.h>
#include <Arduino.h>
#include <stdio.h>


typedef struct {
    char const * const *options;
    uint8_t option_count;
    uint8_t pressed_during_pc;
    uint8_t option_index;
    struct {
        unsigned long int value;
        unsigned long int started_at;
    } animation;
    uint8_t _bools[0];
} ScreenState;
#define SCREEN_SHOULD_EXIT(state) ((state)._bools[0] & 0x01)
#define SCREEN_SET_SHOULD_EXIT(state) (state)._bools[0] |= 0x01
#define SCREEN_CLEAR_SHOULD_EXIT(state) (state)._bools[0] &= ~(0x01)
#define SCREEN_NEEDS_REFRESH(state) (((state)._bools[0] & 0x02) >> 1)
#define SCREEN_SET_NEEDS_REFRESH(state) (state)._bools[0] |= 0x02
#define SCREEN_CLEAR_NEEDS_REFRESH(state) (state)._bools[0] &= ~(0x02)
#define SCREEN_ANIMATION_DURATION_MS 300


static void app_draw(AppState *app_state, ScreenState *screen_state);


uint8_t screen_selector(AppState *app_state, char const * const * options) {
    ScreenState screen_state = {
        .options = options,
        .option_count = 0,
        .option_index = 0,
        .animation = {0}
    };
    while (screen_state.options[screen_state.option_count] != NULL) {
        screen_state.option_count++;
    }
    SCREEN_CLEAR_SHOULD_EXIT(screen_state);
    SCREEN_SET_NEEDS_REFRESH(screen_state);

    if (screen_state.option_count == 0) {
        return 0;
    }

    while (!SCREEN_SHOULD_EXIT(screen_state)) {
        app_input_read(&app_state->input);

        if (screen_state.animation.value > 0) {
            screen_state.animation.value =
                millis() - screen_state.animation.started_at;
            if (screen_state.animation.value > SCREEN_ANIMATION_DURATION_MS) {
                screen_state.animation.value = 0;
                screen_state.animation.started_at = 0;
            }
            SCREEN_SET_NEEDS_REFRESH(screen_state);
        } else if (smart_button_has_raised(app_state->input.push_btns[BTN_B])) {
            screen_state.option_index++;
            screen_state.option_index =
                screen_state.option_index % screen_state.option_count;

            if (screen_state.option_index > 0) {
                screen_state.animation.started_at = millis();
                screen_state.animation.value = 1;
            }

            SCREEN_SET_NEEDS_REFRESH(screen_state);
        } else if (smart_button_has_raised(app_state->input.push_btns[BTN_A])) {
            SCREEN_SET_SHOULD_EXIT(screen_state);
        }

        if (SCREEN_NEEDS_REFRESH(screen_state)) {
            app_draw(app_state, &screen_state);
            SCREEN_CLEAR_NEEDS_REFRESH(screen_state);
        }
    }

    app_input_wait_for_total_release(&app_state->input, NULL, NULL);
    return screen_state.option_index;
};


void app_draw(AppState *app_state, ScreenState *screen_state) {
    static uint8_t const line_height = 16;
    static uint8_t const font_height = 12;
    static uint8_t const font_width = 12;
    static uint8_t const col = 2*font_width;
    u8g2_t *const u8g2 = (u8g2_t *)app_state->display;
    long int animation_pc = 0;
    if (screen_state->animation.value > 0) {
        animation_pc = (
            (screen_state->animation.value << 8)
            / SCREEN_ANIMATION_DURATION_MS
        );
    }
    static int rows[5] = {
        line_height - font_height,
        line_height + 1,
        32 + font_height/2,
        3*line_height + font_height -1,
        3*line_height + 2*font_height
    };

    uint8_t option_index = screen_state->option_index;
    if (screen_state->animation.value && option_index) {
        option_index--;
    }

    u8g2_FirstPage(u8g2);
    do {
        u8g2_ClearBuffer(u8g2);

        if (option_index > 0) {
            u8g2_DrawStr(
                u8g2,
                col,
                (uint8_t)(rows[1] - (((rows[1]-rows[0])*animation_pc)>>8)),
                screen_state->options[option_index-1]
            );
        }
        u8g2_DrawStr(u8g2, font_width/2, rows[2], ">");
        u8g2_DrawStr(
            u8g2, col,
            (uint8_t)(rows[2] - (((rows[2]-rows[1])*animation_pc)>>8)),
            screen_state->options[option_index]
        );
        if (screen_state->option_count > option_index + 0) {
            u8g2_DrawStr(
                u8g2, col,
                (uint8_t)(rows[3] - (((rows[3]-rows[2])*animation_pc)>>8)),
                screen_state->options[option_index+1]
            );
        }

        u8g2_SendBuffer(u8g2);
    } while (u8g2_NextPage(u8g2));
}
