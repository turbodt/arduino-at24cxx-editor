#ifndef INPUT_H
#define INPUT_H


#include "./error.h"
#include "./smart-button.h"


typedef struct {
    SmartButton *push_btns[2];
    unsigned long int pressed_during_ms[2];
} AppInput;


ErrorType app_input_init(AppInput *input);
void app_input_clear(AppInput *input);
void app_input_read(AppInput *input);
void app_input_wait_for_total_release(
    AppInput *input,
    void(*fn)(void *),
    void *user_data
);


#endif
