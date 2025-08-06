#ifndef APP_SMART_BUTTON_H
#define APP_SMART_BUTTON_H


#include <stdint.h>

typedef struct SmartButton SmartButton;
SmartButton * smart_button_make(uint8_t pin, unsigned long repeat_after_ms);
void smart_button_destroy(SmartButton *);

uint8_t smart_button_is_pressed(SmartButton *btn);
uint8_t smart_button_has_raised(SmartButton *btn);
uint8_t smart_button_has_falled(SmartButton *btn);
unsigned long int smart_button_pressed_during_ms(SmartButton *btn);



#endif
