#include "./blank.h"
#include <u8g2.h>
#include <Arduino.h>


void screen_blank(AppState *app_state) {
    u8g2_t *const u8g2 = (u8g2_t *)app_state->display;

    u8g2_FirstPage(u8g2);
    do {
        u8g2_ClearBuffer(u8g2);
    } while (u8g2_NextPage(u8g2));
}
