#include "../config.h"
#include "../helpers.h"
#include "./editor-address.h"
#include <u8g2.h>
#include <Arduino.h>
#include <stdio.h>


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define FONT_HEIGHT 32
#define FONT_WIDTH 20
typedef struct {
    uint8_t digit;
    unsigned int value;
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


unsigned int screen_editor_address(AppState *app_state, unsigned int value) {
    u8g2_t *const u8g2 = (u8g2_t *)app_state->display;

    ScreenState screen_state = {
        .digit = 3,
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
                screen_state.digit = 3;
            }
            SCREEN_SET_NEEDS_REFRESH(screen_state);
        } else if (smart_button_has_falled(app_state->input.push_btns[BTN_A])) {
            uint8_t digits[4] = {
                screen_state.value % 0x10,
                (screen_state.value >> 4) % 0x10,
                (screen_state.value >> 8) % 0x10,
                (screen_state.value >> 12) % 0x10,
            };
            digits[screen_state.digit]++;
            digits[screen_state.digit] = digits[screen_state.digit] % 0x10;
            unsigned int new_value = (digits[3] << 12) + (digits[2] << 8)
                + (digits[1] << 4) + digits[0];
            if (new_value < app_state->eeprom_device.address_count) {
                screen_state.value = new_value;
            } else {
                digits[screen_state.digit] = 0;
                new_value = (digits[3] << 12) + (digits[2] << 8)
                    + (digits[1] << 4) + digits[0];
            }
            screen_state.value = new_value;
            SCREEN_SET_NEEDS_REFRESH(screen_state);
        } else if (
           app_state->input.pressed_during_ms[BTN_A] > PUSH_BTN_LONG_PRESS_MS
        ) {
            SCREEN_SET_SHOULD_EXIT(screen_state);
            SCREEN_SET_NEEDS_REFRESH(screen_state);
        } else if (
           app_state->input.pressed_during_ms[BTN_B] > PUSH_BTN_LONG_PRESS_MS
        ) {
            screen_state.value = value;
            screen_state.digit = 3;
            SCREEN_SET_NEEDS_REFRESH(screen_state);
        } else if (app_state->input.pressed_during_ms[BTN_A] > 0) {
            SCREEN_SET_NEEDS_REFRESH(screen_state);
        } else if (app_state->input.pressed_during_ms[BTN_B] > 0) {
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
    static char buff_byte[8];
    u8g2_t *const u8g2 = (u8g2_t *)app_state->display;
    snprintf(buff_byte, sizeof(buff_byte), "%04X", screen_state->value);

    u8g2_FirstPage(u8g2);
    do {
        u8g2_ClearBuffer(u8g2);

        u8g2_DrawStr(
            u8g2,
            SCREEN_WIDTH/2 - 2*FONT_WIDTH,
            SCREEN_HEIGHT/2 + FONT_HEIGHT/2,
            buff_byte
        );
        u8g2_DrawFrame(
            u8g2,
            SCREEN_WIDTH / 2 + (1-screen_state->digit)*FONT_WIDTH -1,
            SCREEN_HEIGHT/2 - FONT_HEIGHT /2 - 4,
            FONT_WIDTH + 2,
            FONT_HEIGHT + 8
        );

        helper_draw_long_press(u8g2, &app_state->input);

        u8g2_SendBuffer(u8g2);
    } while (u8g2_NextPage(u8g2));
}
