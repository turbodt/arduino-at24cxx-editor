#include "./app-input.h"
#include "./config.h"
#include <Arduino.h>


ErrorType app_input_init(AppInput *input) {
    input->push_btns[BTN_A] = smart_button_make(PUSH_BTN_A_PIN, 0);
    if (input->push_btns[BTN_A] == NULL) {
        goto Button1AllocFailed;
    }

    input->push_btns[BTN_B] = smart_button_make(PUSH_BTN_B_PIN, 0);
    if (input->push_btns[BTN_B] == NULL) {
        goto Button2AllocFailed;
    }

    input->pressed_during_ms[BTN_A] = 0;
    input->pressed_during_ms[BTN_B] = 0;

    return ERR_TYPE__OK;
Button2AllocFailed:
    smart_button_destroy(input->push_btns[BTN_A]);
    input->push_btns[BTN_A] = NULL;
Button1AllocFailed:
    return ERR_TYPE__ALLOC;
};


void app_input_clear(AppInput *input) {
    if (input->push_btns[BTN_A] != NULL) {
        smart_button_destroy(input->push_btns[BTN_A]);
        input->push_btns[BTN_A] = NULL;
    }
    if (input->push_btns[BTN_B] != NULL) {
        smart_button_destroy(input->push_btns[BTN_B]);
        input->push_btns[BTN_B] = NULL;
    }
};


void app_input_read(AppInput *input) {
    input->pressed_during_ms[BTN_A] =
        smart_button_pressed_during_ms(input->push_btns[BTN_A]);
    input->pressed_during_ms[BTN_B] =
        smart_button_pressed_during_ms(input->push_btns[BTN_B]);
};


void app_input_wait_for_total_release(
    AppInput *input,
    void(*fn)(void *),
    void *user_data
) {
    bool should_exit = 0;
    while (!should_exit) {
        app_input_read(input);
        smart_button_has_raised(input->push_btns[BTN_A]);
        smart_button_has_raised(input->push_btns[BTN_B]);
        smart_button_has_falled(input->push_btns[BTN_A]);
        smart_button_has_falled(input->push_btns[BTN_B]);

        should_exit =
            !smart_button_is_pressed(input->push_btns[BTN_A])
            && !smart_button_is_pressed(input->push_btns[BTN_B]);

        if (!should_exit && fn != NULL) {
            fn(user_data);
        }
    }
};
