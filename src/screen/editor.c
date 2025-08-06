#include "../config.h"
#include "../helpers.h"
#include "./editor.h"
#include "./editor-byte.h"
#include "./editor-address.h"
#include "./selector.h"
#include <u8g2.h>
#include <Arduino.h>
#include <stdio.h>


#define CHUNK_COUNT 32
#define COL_COUNT 8
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define FONT_HEIGHT 8
#define FONT_WIDTH 5
#define LINE_HEIGHT 12
#define Y_BODY 10
#define X_ADDR 0
#define Y_ADDR Y_BODY + 2
#define X_VALUES 34
#define Y_VALUES Y_BODY + 2
typedef struct {
    AT24CXX *at24cxx;
    unsigned int address_count;
    unsigned int address;
    unsigned int focus_address;
    unsigned int readed_count;
    uint8_t data[CHUNK_COUNT];
    unsigned long int btn_b_last_hold_action_at;
    uint8_t const *prev_font;
    uint8_t _bools[0];
} ScreenState;
#define SCREEN_SHOULD_EXIT(state) ((state)._bools[0] & 0x01)
#define SCREEN_SET_SHOULD_EXIT(state) (state)._bools[0] |= 0x01
#define SCREEN_CLEAR_SHOULD_EXIT(state) (state)._bools[0] &= ~(0x01)
#define SCREEN_NEEDS_REFRESH(state) (((state)._bools[0] & 0x02) >> 1)
#define SCREEN_SET_NEEDS_REFRESH(state) (state)._bools[0] |= 0x02
#define SCREEN_CLEAR_NEEDS_REFRESH(state) (state)._bools[0] &= ~(0x02)
#define SCREEN_BTN_A_LONG_PRESS(state) (((state)._bools[0] & 0x04) >> 2)
#define SCREEN_SET_A_LONG_PRESS(state) (state)._bools[0] |= 0x04
#define SCREEN_CLEAR_A_LONG_PRESS(state) (state)._bools[0] &= ~(0x04)
#define SCREEN_BTN_B_LONG_PRESS(state) (((state)._bools[0] & 0x08) >> 3)
#define SCREEN_SET_B_LONG_PRESS(state) (state)._bools[0] |= 0x08
#define SCREEN_CLEAR_B_LONG_PRESS(state) (state)._bools[0] &= ~(0x08)


static u8g2_t * u8g2;
static char buff_byte[8];
static char const * const main_options[] = {
    "Set address",
    "Back",
    "Exit",
    NULL,
};
static void btn_a_pressed(AppState *app_state, ScreenState *screen_state);
static void btn_b_pressed(AppState *app_state, ScreenState *screen_state);
static void btn_a_long_pressed(AppState *app_state, ScreenState *screen_state);
static void btn_b_long_pressed(AppState *app_state, ScreenState *screen_state);
static void load_from_address(
    AppState *app_state,
    ScreenState *screen_state,
    unsigned int address
);
static void modify_value(AppState *app_state, ScreenState *screen_state);
static void app_draw(AppState *app_state, ScreenState *screen_state);
static void app_draw_addresses(AppState *app_state, ScreenState *screen_state);
static void app_draw_values(AppState *app_state, ScreenState *screen_state);


void screen_editor(AppState *app_state) {
    static bool btn_has_falled[2];
    u8g2 = (u8g2_t *)app_state->display;

    ScreenState screen_state = {
        .at24cxx = NULL,
        .address = 0,
        .focus_address = 0,
        .prev_font = u8g2->font,
        .btn_b_last_hold_action_at = 0,
    };

    u8g2_SetFont(u8g2, u8g2_font_5x8_tf);

    // at24cxx
    TwoWires *tw = two_wires_get();
    screen_state.at24cxx = at24cxx_make_64();
    if (screen_state.at24cxx==NULL) {
        app_state->err = ERR_TYPE__ALLOC;
        goto AT24CXXMakeFailed;
    }
    at24cxx_begin(screen_state.at24cxx, EEPROM_ADDR, tw);
    load_from_address(app_state, &screen_state, 0);

    SCREEN_CLEAR_A_LONG_PRESS(screen_state);
    SCREEN_CLEAR_B_LONG_PRESS(screen_state);
    SCREEN_CLEAR_SHOULD_EXIT(screen_state);
    SCREEN_SET_NEEDS_REFRESH(screen_state);

    // body
    while (!SCREEN_SHOULD_EXIT(screen_state)) {
        app_input_read(&app_state->input);
        btn_has_falled[BTN_A] =
            smart_button_has_falled(app_state->input.push_btns[BTN_A]);
        btn_has_falled[BTN_B] =
            smart_button_has_falled(app_state->input.push_btns[BTN_B]);

        if (btn_has_falled[BTN_B] && SCREEN_BTN_B_LONG_PRESS(screen_state)) {
            btn_b_long_pressed(app_state,&screen_state);
        } else if (
            btn_has_falled[BTN_A] && SCREEN_BTN_A_LONG_PRESS(screen_state)
        ) {
            btn_a_long_pressed(app_state,&screen_state);
        } else if (btn_has_falled[BTN_A]) {
            btn_a_pressed(app_state,&screen_state);
        } else if (btn_has_falled[BTN_B]) {
            btn_b_pressed(app_state,&screen_state);
        } else if (
            app_state->input.pressed_during_ms[BTN_A] > PUSH_BTN_LONG_PRESS_MS
            && !SCREEN_BTN_A_LONG_PRESS(screen_state)
        ) {
            SCREEN_SET_A_LONG_PRESS(screen_state);
        } else if (
            app_state->input.pressed_during_ms[BTN_B] > PUSH_BTN_LONG_PRESS_MS
            && !SCREEN_BTN_B_LONG_PRESS(screen_state)
        ) {
            SCREEN_SET_B_LONG_PRESS(screen_state);
        } else if (
            app_state->input.pressed_during_ms[BTN_B] > PUSH_BTN_LONG_PRESS_MS
            && screen_state.btn_b_last_hold_action_at == 0
        ) {
            screen_state.focus_address = screen_state.address;
            screen_state.btn_b_last_hold_action_at = millis();
            SCREEN_SET_NEEDS_REFRESH(screen_state);
        } else if (
            app_state->input.pressed_during_ms[BTN_B] > PUSH_BTN_LONG_PRESS_MS
            && (millis() - screen_state.btn_b_last_hold_action_at
                > PUSH_BTN_LONG_PRESS_MS)
        ) {
            unsigned int target_address;
            target_address = (screen_state.address + CHUNK_COUNT)
                % app_state->eeprom_device.address_count;
            load_from_address(app_state, &screen_state, target_address);

            screen_state.btn_b_last_hold_action_at = millis();
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

    at24cxx_destroy(screen_state.at24cxx);
    screen_state.at24cxx = NULL;
AT24CXXMakeFailed:
    u8g2_SetFont(u8g2, screen_state.prev_font);
    app_input_wait_for_total_release(&app_state->input, NULL, NULL);
    return;
};

inline void btn_a_pressed(AppState *app_state, ScreenState *screen_state) {
    (void)app_state;
    modify_value(app_state, screen_state);
    SCREEN_CLEAR_A_LONG_PRESS(*screen_state);
    SCREEN_SET_NEEDS_REFRESH(*screen_state);
};


inline void btn_b_pressed(AppState *app_state, ScreenState *screen_state) {
    (void)app_state;
    if (screen_state->btn_b_last_hold_action_at == 0) {
        screen_state->focus_address++;
        if (screen_state->focus_address
            >= screen_state->address + CHUNK_COUNT
        ) {
            screen_state->focus_address = screen_state->address;
        }
    } else {
        screen_state->btn_b_last_hold_action_at = 0;
    }
    SCREEN_CLEAR_B_LONG_PRESS(*screen_state);
    SCREEN_SET_NEEDS_REFRESH(*screen_state);
};


inline void btn_a_long_pressed(AppState *app_state, ScreenState *screen_state) {
    app_draw(app_state, screen_state);
    app_input_wait_for_total_release(&app_state->input, NULL, NULL);
    uint8_t option_index = screen_selector(app_state, main_options);
    if (option_index == 0) {
        unsigned int target_address = screen_editor_address(
            app_state,
            screen_state->focus_address
        );
        load_from_address(app_state, screen_state, target_address);
        screen_state->focus_address = target_address;
    } else if (option_index == 2) {
        SCREEN_SET_SHOULD_EXIT(*screen_state);
    }
    SCREEN_CLEAR_A_LONG_PRESS(*screen_state);
    SCREEN_SET_NEEDS_REFRESH(*screen_state);
}


inline void btn_b_long_pressed(AppState *app_state, ScreenState *screen_state) {
    (void)app_state;
    screen_state->btn_b_last_hold_action_at = 0;
    SCREEN_CLEAR_B_LONG_PRESS(*screen_state);
    SCREEN_SET_NEEDS_REFRESH(*screen_state);
}


inline void load_from_address(
    AppState *app_state,
    ScreenState *screen_state,
    unsigned int address
) {
    uint8_t count = CHUNK_COUNT;
    screen_state->address = address - (address % CHUNK_COUNT);
    count = min(
        app_state->eeprom_device.address_count - screen_state->address,
        CHUNK_COUNT
    );
    screen_state->readed_count = at24cxx_read_at(
        screen_state->at24cxx,
        screen_state->address,
        screen_state->data,
        count
    );
    screen_state->focus_address = address;
}


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
    static char buff_byte[8];
    sprintf(buff_byte, "0x%04X", screen_state->focus_address);

    u8g2_FirstPage(u8g2);
    do {
        u8g2_ClearBuffer(u8g2);

        app_draw_addresses(app_state, screen_state);
        app_draw_values(app_state, screen_state);
        u8g2_DrawStr(
            u8g2,
            SCREEN_WIDTH - 6*FONT_WIDTH,
            FONT_HEIGHT,
            buff_byte
        );
        u8g2_DrawVLine(u8g2, SCREEN_WIDTH -6*FONT_WIDTH - 2, 0, Y_BODY);
        u8g2_DrawVLine(u8g2, 6*FONT_WIDTH + 1, Y_ADDR -2 , 3*LINE_HEIGHT + FONT_HEIGHT + 4);
        u8g2_DrawHLine(u8g2, 0, Y_BODY, SCREEN_WIDTH);
        u8g2_DrawHLine(u8g2, 0, Y_BODY + 3*LINE_HEIGHT + FONT_HEIGHT + 4, SCREEN_WIDTH);

        helper_draw_long_press(u8g2, &app_state->input);

        u8g2_SendBuffer(u8g2);
    } while (u8g2_NextPage(u8g2));
}

inline void app_draw_addresses(AppState *app_state, ScreenState *screen_state) {
    for (uint8_t i = 0; i*COL_COUNT < screen_state->readed_count; i++) {
        sprintf(buff_byte, "0x%04X", screen_state->address + i*COL_COUNT);
        u8g2_DrawStr(
            u8g2,
            X_ADDR,
            Y_VALUES + i*LINE_HEIGHT + FONT_HEIGHT,
            buff_byte
        );
    }
}

inline void app_draw_values(AppState *app_state, ScreenState *screen_state) {
    uint8_t offset = 0;
    for (uint8_t i = 0; i*COL_COUNT < screen_state->readed_count; i++) {
        for (
            uint8_t j = 0;
            j < COL_COUNT && j +i*COL_COUNT < screen_state->readed_count;
            j++
        ) {
            offset = i*COL_COUNT + j;
            sprintf(buff_byte, "%02X", screen_state->data[offset]);
            u8g2_DrawStr(
                u8g2,
                X_VALUES + j*2*FONT_WIDTH + 2*j,
                Y_VALUES + i*LINE_HEIGHT + FONT_HEIGHT,
                buff_byte
            );
            if (
                screen_state->focus_address - screen_state->address
                == offset
            ) {
                u8g2_DrawFrame(
                    u8g2,
                    X_VALUES + j*2*FONT_WIDTH + 2*j - 2,
                    Y_VALUES + i*LINE_HEIGHT -2,
                    2*FONT_WIDTH +2,
                    FONT_HEIGHT +4
                );
            }
        }
    }
}
