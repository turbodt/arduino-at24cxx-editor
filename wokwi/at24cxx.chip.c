// Wokwi Custom Chip - For information and examples see:
// https://link.wokwi.com/custom-chips-alpha
//
// SPDX-License-Identifier: MIT
// Copyright (C) 2022 Uri Shaked / wokwi.com

#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const int BASE_ADDRESS = 0x50;

typedef enum {
  WRITE_CYCLE,
  IDLE,
  ADDRESS_1,
  DATA
} eeprom_state_t;

typedef struct {
  eeprom_state_t state;
  pin_t pin_a0;
  pin_t pin_a1;
  pin_t pin_a2;
  pin_t pin_wp;
  uint16_t address_register;
  uint8_t *buff;
  uint8_t *mem;
  uint8_t byte_counter;
} chip_state_t;


static bool on_i2c_connect(void *user_data, uint32_t address, bool connect);
static uint8_t on_i2c_read(void *user_data);
static bool on_i2c_write(void *user_data, uint8_t data);
static void on_i2c_disconnect(void *user_data);

uint32_t eepromSize;
uint8_t pageSize;
timer_t writeTimer;

static uint8_t get_addr(chip_state_t *chip) {
  uint8_t address = BASE_ADDRESS;
  address |= pin_read(chip->pin_a2) << 2;
  address |= pin_read(chip->pin_a1) << 1;
  address |= pin_read(chip->pin_a0);
  return address;
}

void write_eeprom(void *user_data) {
  chip_state_t *chip = user_data;
  for(int i = 0; i < chip->byte_counter; i++){
    if(pin_read(chip->pin_wp)==LOW || chip->address_register <= eepromSize*3/4-1)
      chip->mem[chip->address_register] = chip->buff[i];
    if(!((chip->address_register+1) & (pageSize-1)))
      chip->address_register &= 0xffff-(pageSize-1);
    else chip->address_register++;
  }
  printf("Memory dump:");
  for(int i = 0; i < eepromSize && i < 4*pageSize; i++) {
    if (i%pageSize == 0) {
      printf("\nPage %d",i/pageSize + 1);
    }
    if (i%16 == 0) {
      printf("\n0x%04x:", i);
    }
    printf(" 0x%02x", chip->mem[i]);
  }
  printf("\n");
  chip->byte_counter = 0;
  chip->state = IDLE;
}

void chip_init(void) {
  chip_state_t *chip = malloc(sizeof(chip_state_t));
  uint32_t eepromSizeAttr = attr_init("eepromSizeKb", 32);
  switch(attr_read(eepromSizeAttr)){
    case 32:
    case 64: pageSize = 32; break;
    case 128:
    case 256: pageSize = 64; break;
    case 512: pageSize = 128;
  }
  eepromSize = attr_read(eepromSizeAttr)/8*1024;
  chip->pin_a0 = pin_init("A0", INPUT);
  chip->pin_a1 = pin_init("A1", INPUT);
  chip->pin_a2 = pin_init("A2", INPUT);
  chip->pin_wp = pin_init("WP", INPUT);
  chip->buff = malloc(pageSize);
  chip->mem = malloc(eepromSize);
  uint8_t address = get_addr(chip);
  chip->address_register = 0;
  chip->byte_counter = 0;
  memset(chip->mem, 255, eepromSize);
  chip->state = IDLE;

    const i2c_config_t i2c_config = {
    .user_data = chip,
    .address = address,
    .scl = pin_init("SCL", INPUT),
    .sda = pin_init("SDA", INPUT),
    .connect = on_i2c_connect,
    .read = on_i2c_read,
    .write = on_i2c_write,
    .disconnect = on_i2c_disconnect, // Optional
  };
  i2c_init(&i2c_config);

  timer_config_t tconf = {
    .callback = write_eeprom,
    .user_data = chip,
  };
  writeTimer = timer_init(&tconf);

  // The following message will appear in the browser's DevTools console:
  printf("at24cxx at address 0x%x\neeprom size: %d Byte\npage size: %d Byte\n", address, eepromSize, pageSize);
}


bool on_i2c_connect(void *user_data, uint32_t address, bool connect) {
  //printf("on_i2c_connect...\n");
  chip_state_t *chip = user_data;
  if(chip->state == WRITE_CYCLE) return false;
  //printf("Success!\n");
  return true; /* Ack */
}

uint8_t on_i2c_read(void *user_data) {
  //printf("on_i2c_read\n");
  chip_state_t *chip = user_data;
  uint8_t data = chip->mem[chip->address_register];
  //printf("readed data %d('%c') at 0x%x\n",data,data,chip->address_register);
  chip->address_register = (chip->address_register + 1) & (eepromSize-1);
  return data;
}

bool on_i2c_write(void *user_data, uint8_t data) {
  printf("on_i2c_write\n");
  chip_state_t *chip = user_data;
  switch(chip->state) 
  {
    case IDLE:
      chip->address_register = (data & 0xf) << 8;
      chip->state = ADDRESS_1;
    break;
    case ADDRESS_1:
      chip->address_register |= data;
      chip->address_register &= eepromSize-1;
      chip->state = DATA;
    break;
    case DATA:
      if (chip->byte_counter > pageSize-1) {
        chip->state = IDLE;
        return false;
      }
      chip->buff[chip->byte_counter] = data;
      printf("Wroted %d ('%c') at '0x%x'\n",data,data,chip->byte_counter);
      chip->byte_counter++;
      //chip->address_register = (chip->address_register & 0xfe0) + (chip->address_register & 0x1f) + 1 & 0x1f
    break;
    default:
      printf("error");
    break;
  } 
    return true; // Ack
}

void on_i2c_disconnect(void *user_data) {
  chip_state_t *chip = user_data;
  if(chip->state == WRITE_CYCLE) return;
  if(chip->byte_counter > 0){
    timer_start(writeTimer,chip->byte_counter * 312,false);  // One byte takes max 312 microseconds
    chip->state = WRITE_CYCLE;
  }
  else chip->state = IDLE;
}
