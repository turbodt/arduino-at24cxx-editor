#include "../config.h"
#include "../helpers.h"
#include "./editor.h"
#include "./editor-byte.h"
#include <u8g2.h>
#include <Arduino.h>
#include <stdio.h>


#define CHUNK_COUNT 32
typedef struct {
    AT24CXX *at24cxx;
    unsigned int address;
    unsigned int focus_address;
    unsigned int readed_count;
    uint8_t data[CHUNK_COUNT];
    uint8_t const *prev_font;
    uint8_t _bools[0];
} ScreenState;
#define SCREEN_SHOULD_EXIT(state) ((state)._bools[0] & 0x01)
#define SCREEN_SET_SHOULD_EXIT(state) (state)._bools[0] |= 0x01
#define SCREEN_CLEAR_SHOULD_EXIT(state) (state)._bools[0] &= ~(0x01)
#define SCREEN_NEEDS_REFRESH(state) (((state)._bools[0] & 0x02) >> 1)
#define SCREEN_SET_NEEDS_REFRESH(state) (state)._bools[0] |= 0x02
#define SCREEN_CLEAR_NEEDS_REFRESH(state) (state)._bools[0] &= ~(0x02)


static void modify_value(AppState *app_state, ScreenState *screen_state);
static void app_draw(AppState *app_state, ScreenState *screen_state);


void screen_editor(AppState *app_state) {
    u8g2_t *const u8g2 = (u8g2_t *)app_state->display;

    ScreenState screen_state = {
        .at24cxx = NULL,
        .address = 0,
        .focus_address = 0,
        .prev_font = u8g2->font,
    };
    SCREEN_CLEAR_SHOULD_EXIT(screen_state);
    SCREEN_SET_NEEDS_REFRESH(screen_state);

    u8g2_SetFont(u8g2, u8g2_font_5x8_tf);

    // at24cxx
    TwoWires *tw = two_wires_get();
    screen_state.at24cxx = at24cxx_make_64();
    if (screen_state.at24cxx==NULL) {
        app_state->err = ERR_TYPE__ALLOC;
        goto AT24CXXMakeFailed;
    }
    at24cxx_begin(screen_state.at24cxx, EEPROM_ADDR, tw);
    screen_state.readed_count = at24cxx_read_at(
        screen_state.at24cxx,
        screen_state.address,
        screen_state.data,
        CHUNK_COUNT
    );
    SCREEN_SET_NEEDS_REFRESH(screen_state);

    // body
    while (!SCREEN_SHOULD_EXIT(screen_state)) {
        app_input_read(&app_state->input);

        if (smart_button_has_falled(app_state->input.push_btns[BTN_B])) {
            screen_state.focus_address++;
            if (
                screen_state.focus_address
                >= screen_state.address + CHUNK_COUNT
            ) {
                screen_state.focus_address = screen_state.address;
            }
            SCREEN_SET_NEEDS_REFRESH(screen_state);
        } else if (smart_button_has_falled(app_state->input.push_btns[BTN_A])) {
            modify_value(app_state, &screen_state);
            SCREEN_SET_NEEDS_REFRESH(screen_state);
        } else if (
           app_state->input.pressed_during_ms[BTN_A] > PUSH_BTN_LONG_PRESS_MS
        ) {
            SCREEN_SET_NEEDS_REFRESH(screen_state);
            SCREEN_SET_SHOULD_EXIT(screen_state);
        } else if (app_state->input.pressed_during_ms[BTN_A] > 0) {
            SCREEN_SET_NEEDS_REFRESH(screen_state);
        }

        if (SCREEN_NEEDS_REFRESH(screen_state)) {
            app_draw(app_state, &screen_state);
            SCREEN_CLEAR_NEEDS_REFRESH(screen_state);
        }
    }

    at24cxx_destroy(screen_state.at24cxx);
    screen_state.at24cxx = NULL;
AT24CXXMakeFailed:
    u8g2_SetFont(u8g2, screen_state.prev_font);
    app_input_wait_for_total_release(&app_state->input, NULL, NULL);
    return;
};


inline void modify_value(AppState *app_state, ScreenState *screen_state) {
    uint8_t value = screen_editor_byte(
        app_state,
        screen_state->data[
            screen_state->focus_address - screen_state->address
        ]
    );

    bool write_success = at24cxx_write_byte_at(
        screen_state->at24cxx,
        screen_state->focus_address,
        value
    );

    if (write_success) {
        screen_state->data[
            screen_state->focus_address - screen_state->address
        ] = value;
    }
};


void app_draw(AppState *app_state, ScreenState *screen_state) {
    static uint8_t const col_count = 8;
    static uint8_t const font_height = 8;
    static char buff_byte[8];
    u8g2_t *const u8g2 = (u8g2_t *)app_state->display;

    u8g2_FirstPage(u8g2);
    do {
        u8g2_ClearBuffer(u8g2);

        uint8_t offset = 0;
        for (uint8_t i = 0; i*col_count < CHUNK_COUNT; i++) {
            offset = i*col_count;
            sprintf(buff_byte, "0x%04X", screen_state->address + offset);
            u8g2_DrawStr(u8g2, 0, 4 + i*12 + font_height, buff_byte);
            for (
                uint8_t j = 0;
                j < col_count && j +i*col_count < CHUNK_COUNT;
                j++
            ) {
                offset = i*col_count + j;
                sprintf(buff_byte, "%02X", screen_state->data[offset]);
                u8g2_DrawStr(
                    u8g2,
                    34 + j*2*5 + 2*j,
                    4 + i*12 + font_height,
                    buff_byte
                );
                if (
                    screen_state->focus_address - screen_state->address
                    == offset
                ) {
                    u8g2_DrawFrame(
                        u8g2,
                        32 + j*2*5 + 2*j,
                        2 + i*12,
                        2*5 + 2,
                        font_height+4
                    );
                }
            }
        }

        u8g2_DrawVLine(u8g2, 31, 0, 48 + 2);
        u8g2_DrawHLine(u8g2, 0, 48 + 2, 128);

        helper_draw_long_press(u8g2, &app_state->input);

        u8g2_SendBuffer(u8g2);
    } while (u8g2_NextPage(u8g2));
}
