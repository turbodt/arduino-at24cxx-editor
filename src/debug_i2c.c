#include "./debug.h"
#include "./config.h"
#include <Arduino.h>


void debug_i2c_init(void) {
    if (!DEBUG_WOKWI_I2C_ANALYSER_PIN) {
        return;
    }
    pinMode(DEBUG_WOKWI_I2C_ANALYSER_PIN, OUTPUT);
    digitalWrite(DEBUG_WOKWI_I2C_ANALYSER_PIN, LOW);
};


void debug_i2c_start(void) {
    if (!DEBUG_WOKWI_I2C_ANALYSER_PIN) {
        return;
    }
    digitalWrite(DEBUG_WOKWI_I2C_ANALYSER_PIN, HIGH);
};


void debug_i2c_stop(void) {
    if (!DEBUG_WOKWI_I2C_ANALYSER_PIN) {
        return;
    }
    digitalWrite(DEBUG_WOKWI_I2C_ANALYSER_PIN, LOW);
};
