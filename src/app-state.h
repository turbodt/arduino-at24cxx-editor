#ifndef APP_STATE_H
#define APP_STATE_H


#include <at24cxx.h>
#include "./error.h"
#include "./display.h"
#include "./app-input.h"


typedef struct AppState {
    ErrorType err;
    AppInput input;
    Display *display;
    uint8_t data_chunk[16];
} AppState;


ErrorType app_state_init(AppState *);
ErrorType app_state_clear(AppState *);


#endif
