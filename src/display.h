#ifndef APP_DISPLAY_H
#define APP_DISPLAY_H


#include <stdint.h>


typedef struct Display Display;


Display *display_make(uint8_t pin_clock, uint8_t pin_data);
void display_destroy(Display*);


#endif
