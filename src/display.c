#include "./display.h"
#include <Arduino.h>
#include <two_wires.h>
#include <u8g2.h>


struct Display {
    u8g2_t base;
};

static uint8_t u8x8_gpio_and_delay_arduino(
    u8x8_t *u8x8,
    uint8_t msg,
    uint8_t arg_int,
    U8X8_UNUSED void *arg_ptr
);
static uint8_t u8x8_byte_arduino_hw_i2c(
    U8X8_UNUSED u8x8_t *u8x8,
    U8X8_UNUSED uint8_t msg,
    U8X8_UNUSED uint8_t arg_int,
    U8X8_UNUSED void *arg_ptr
);


Display *display_make(uint8_t pin_clock, uint8_t pin_data) {

    Display *display = malloc(sizeof(Display));
    if (display == NULL) {
        return NULL;
    }

    u8g2_Setup_ssd1306_i2c_128x64_noname_2(
        &display->base,
        U8G2_R0,
        //u8x8_byte_sw_i2c,
        u8x8_byte_arduino_hw_i2c,  // x10 faster
        u8x8_gpio_and_delay_arduino
    );

    u8x8_t * u8x8 = u8g2_GetU8x8(&display->base);
    u8x8_SetPin(u8x8, U8X8_PIN_RESET, U8X8_PIN_NONE);
    u8x8_SetPin(u8x8, U8X8_PIN_I2C_CLOCK, pin_clock);
    u8x8_SetPin(u8x8, U8X8_PIN_I2C_DATA, pin_data);

    u8x8_utf8_init(u8x8);

    u8g2_InitDisplay(&display->base);
    u8g2_ClearDisplay(&display->base);
    u8g2_SetPowerSave(&display->base, 0);

    //u8g2_InitInterface(&display->base);
    //u8g2_ClearDisplay(&display->base);
    u8g2_SetFont(&display->base, u8g2_font_helvB12_tr);
    //u8g2_SetFont(&display->base, u8g2_font_5x8_tf);
    u8g2_SetFontMode(&display->base, 1);
    u8g2_SetDrawColor(&display->base, 1);

    return display;
};


void display_destroy(Display*display) {
    free(display);
};


#ifdef U8X8_USE_PINS
uint8_t u8x8_gpio_and_delay_arduino(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, U8X8_UNUSED void *arg_ptr)
{
  uint8_t i;
  switch(msg)
  {
    case U8X8_MSG_GPIO_AND_DELAY_INIT:
    
      for( i = 0; i < U8X8_PIN_CNT; i++ )
	if ( u8x8->pins[i] != U8X8_PIN_NONE )
	{
	  if ( i < U8X8_PIN_OUTPUT_CNT )
	  {
	    pinMode(u8x8->pins[i], OUTPUT);
	  }
	  else
	  {
#ifdef INPUT_PULLUP
	    pinMode(u8x8->pins[i], INPUT_PULLUP);
#else
	    pinMode(u8x8->pins[i], OUTPUT);
	    digitalWrite(u8x8->pins[i], 1);
#endif 
	  }
	}
	  
      break;

#ifndef __AVR__	
    /* this case is not compiled for any AVR, because AVR uC are so slow */
    /* that this delay does not matter */
    case U8X8_MSG_DELAY_NANO:
      delayMicroseconds(arg_int==0?0:1);
      break;
#endif
    
    case U8X8_MSG_DELAY_10MICRO:
      /* not used at the moment */
      break;
    
    case U8X8_MSG_DELAY_100NANO:
      /* not used at the moment */
      break;
   
    case U8X8_MSG_DELAY_MILLI:
      delay(arg_int);
      break;
    case U8X8_MSG_DELAY_I2C:
      /* arg_int is 1 or 4: 100KHz (5us) or 400KHz (1.25us) */
      delayMicroseconds(arg_int<=2?5:2);
      break;
    case U8X8_MSG_GPIO_I2C_CLOCK:
    case U8X8_MSG_GPIO_I2C_DATA:
      if ( arg_int == 0 )
      {
	pinMode(u8x8_GetPinValue(u8x8, msg), OUTPUT);
	digitalWrite(u8x8_GetPinValue(u8x8, msg), 0);
      }
      else
      {
#ifdef INPUT_PULLUP
	pinMode(u8x8_GetPinValue(u8x8, msg), INPUT_PULLUP);
#else
	pinMode(u8x8_GetPinValue(u8x8, msg), OUTPUT);
	digitalWrite(u8x8_GetPinValue(u8x8, msg), 1);
#endif 
      }
      break;
    default:
      if ( msg >= U8X8_MSG_GPIO(0) )
      {
	i = u8x8_GetPinValue(u8x8, msg);
	if ( i != U8X8_PIN_NONE )
	{
	  if ( u8x8_GetPinIndex(u8x8, msg) < U8X8_PIN_OUTPUT_CNT )
	  {
	    digitalWrite(i, arg_int);
	  }
	  else
	  {
	    if ( u8x8_GetPinIndex(u8x8, msg) == U8X8_PIN_OUTPUT_CNT )
	    {
	      // call yield() for the first pin only, u8x8 will always request all the pins, so this should be ok
	      yield();
	    }
	    u8x8_SetGPIOResult(u8x8, digitalRead(i) == 0 ? 0 : 1);
	  }
	}
	break;
      }
        return 0;
    }
    return 1;
}
#endif


static uint8_t u8x8_byte_arduino_hw_i2c(
    U8X8_UNUSED u8x8_t *u8x8,
    U8X8_UNUSED uint8_t msg,
    U8X8_UNUSED uint8_t arg_int,
    U8X8_UNUSED void *arg_ptr
) {

    struct TwoWires * two_wires = two_wires_get();

    switch(msg)
    {
        case U8X8_MSG_BYTE_SEND:
            two_wires->write(two_wires, (uint8_t *)arg_ptr, (size_t)arg_int);
        break;
        case U8X8_MSG_BYTE_INIT:
            if ( u8x8->bus_clock == 0 ) {
                u8x8->bus_clock = u8x8->display_info->i2c_bus_clock_100kHz * 100000UL;
            }
            two_wires->begin(two_wires);
        break;
        case U8X8_MSG_BYTE_SET_DC:
        break;
        case U8X8_MSG_BYTE_START_TRANSFER:
#if ARDUINO >= 10600
        /* not sure when the setClock function was introduced, but it is there since 1.6.0 */
        /* if there is any error with Wire.setClock() just remove this function call by */
        /* defining U8X8_DO_NOT_SET_WIRE_CLOCK */
#ifndef U8X8_DO_NOT_SET_WIRE_CLOCK
            two_wires->set_clock(two_wires, u8x8->bus_clock);
#endif
#endif
            two_wires->begin_transmission(
                two_wires,
                u8x8_GetI2CAddress(u8x8)>>1
            );
        break;
        case U8X8_MSG_BYTE_END_TRANSFER:
            two_wires->end_transmission(two_wires);
        break;
        default:
            return 0;
    }
    return 1;
}
