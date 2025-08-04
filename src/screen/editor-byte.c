#include "../config.h"
#include "./editor-byte.h"
#include <u8g2.h>
#include <Arduino.h>
#include <stdio.h>


typedef struct {
    uint8_t digit;
    uint8_t value;
    uint8_t const * prev_font;
    uint8_t _bools[0];
} ScreenState;
#define SCREEN_SHOULD_EXIT(state) ((state)._bools[0] & 0x01)
#define SCREEN_SET_SHOULD_EXIT(state) (state)._bools[0] |= 0x01
#define SCREEN_CLEAR_SHOULD_EXIT(state) (state)._bools[0] &= ~(0x01)
#define SCREEN_NEEDS_REFRESH(state) (((state)._bools[0] & 0x02) >> 1)
#define SCREEN_SET_NEEDS_REFRESH(state) (state)._bools[0] |= 0x02
#define SCREEN_CLEAR_NEEDS_REFRESH(state) (state)._bools[0] &= ~(0x02)


static void app_draw(AppState *app_state, ScreenState *screen_state);


uint8_t screen_editor_byte(AppState *app_state, uint8_t value) {
    u8g2_t *const u8g2 = (u8g2_t *)app_state->display;

    ScreenState screen_state = {
        .digit = 1,
        .value = value,
        .prev_font = u8g2->font,
    };
    SCREEN_CLEAR_SHOULD_EXIT(screen_state);
    SCREEN_SET_NEEDS_REFRESH(screen_state);

    u8g2_SetFont(u8g2, u8g2_font_logisoso32_tf);

    // body
    while (!SCREEN_SHOULD_EXIT(screen_state)) {
        app_input_read(&app_state->input);

        if (smart_button_has_falled(app_state->input.push_btns[BTN_B])) {
            if (screen_state.digit > 0) {
                screen_state.digit--;
            } else {
                SCREEN_SET_SHOULD_EXIT(screen_state);
            }
            SCREEN_SET_NEEDS_REFRESH(screen_state);
        } else if (smart_button_has_falled(app_state->input.push_btns[BTN_A])) {
            uint8_t digits[2] = {
                screen_state.value % 0x10,
                (screen_state.value >> 4) % 0x10,
            };
            digits[screen_state.digit]++;
            digits[screen_state.digit] = digits[screen_state.digit] % 0x10;
            screen_state.value = (digits[1] << 4) + digits[0];
            SCREEN_SET_NEEDS_REFRESH(screen_state);
        }

        if (SCREEN_NEEDS_REFRESH(screen_state)) {
            app_draw(app_state, &screen_state);
            SCREEN_CLEAR_NEEDS_REFRESH(screen_state);
        }
    }

    u8g2_SetFont(u8g2, screen_state.prev_font);
    app_input_wait_for_total_release(&app_state->input, NULL, NULL);
    return screen_state.value;
}


void app_draw(AppState *app_state, ScreenState *screen_state) {
    static uint8_t const font_height = 32;
    static uint8_t const font_width = 20;
    static char buff_byte[8];
    u8g2_t *const u8g2 = (u8g2_t *)app_state->display;
    snprintf(buff_byte, sizeof(buff_byte), "0x%02X", screen_state->value);

    u8g2_FirstPage(u8g2);
    do {
        u8g2_ClearBuffer(u8g2);

        u8g2_DrawStr(u8g2, 64 - 2*font_width, 32 + font_height/2, buff_byte);
        u8g2_DrawFrame(
            u8g2,
            64 + (1-screen_state->digit)*font_width -1,
            32 - font_height /2 - 4,
            font_width + 2,
            font_height + 8
        );

        u8g2_SendBuffer(u8g2);
    } while (u8g2_NextPage(u8g2));
}
