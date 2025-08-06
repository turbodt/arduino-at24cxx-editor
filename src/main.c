#include <Arduino.h>
#include <u8g2.h>
#include "./config.h"
#include "./debug.h"
#include "./app-state.h"
#include "./screen/main.h"
#include <stdio.h>


static char const * const available_chips[] = {
    "AT24C32",
    "AT24C64",
    "AT24C128",
    "AT24C256",
    NULL
};
static unsigned int const page_sizes[] = {32, 32, 64, 64, 128};
static unsigned int const page_counts[] = {128, 256, 256, 512, 512};


int main(void) {
    init();
    pinMode(8, OUTPUT);
    digitalWrite(8, HIGH);
    debug_i2c_init();

    static AppState state = {0};
    app_state_init(&state);
    if (state.err!=ERR_TYPE__OK) {
        goto AppStateInitFailed;
    }

    while(1) {
        app_input_wait_for_total_release(&state.input, NULL, NULL);
        screen_start(&state);
        uint8_t device_index = screen_selector(&state, available_chips);
        state.eeprom_device.page_size = page_sizes[device_index];
        state.eeprom_device.page_count = page_counts[device_index];
        screen_editor(&state);
    }

    app_state_clear(&state);
    return 0;
AppStateInitFailed:
    return 1;
}
