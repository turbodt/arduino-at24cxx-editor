#include "./config.h"
#include "./app-state.h"
#include "./display.h"
#include <Arduino.h>
#include <two_wires.h>



ErrorType app_state_init(AppState *state) {
    // Display
    state->display = display_make(I2C_SCL, I2C_SDA);
    if (state->display==NULL) {
        state->err = ERR_TYPE__ALLOC;
        goto DisplayMakeFailed;
    }

    // input
    state->err = app_input_init(&state->input);
    if (state->err != ERR_TYPE__OK) {
        goto AppInputInitFailed;
    }

    // others
    memset(state->data_chunk, 0, sizeof(state->data_chunk));

    // return
    state->err = ERR_TYPE__OK;
    return ERR_TYPE__OK;
AppInputInitFailed:
    display_destroy(state->display);
    state->display = NULL;
DisplayMakeFailed:
    return ERR_TYPE__ALLOC;
};


ErrorType app_state_clear(AppState *state) {
    if (state->display != NULL) {
        display_destroy(state->display);
        state->display = NULL;
    }

    app_input_clear(&state->input);

    return ERR_TYPE__OK;
};
