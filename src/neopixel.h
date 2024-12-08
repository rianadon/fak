// NeoPixel (Addressable LED) Functions for CH551, CH552 and CH554
// CC-BY-SA-3.0
// Version 1.2 by Stefan Wagner (https://github.com/wagiminator)
// Version 1.3 adapted for FAK firmware


#pragma once
#include <stdint.h>
#include <stddef.h>

// typedef struct {
//   sbit pin;
//   uint8_t  count;
//   uint24_t *buffer;
// } neopixel_def_t;

// #define neopixel_latch() DLY_us(281) // latch colors

void neopixel_sendByte(uint8_t data);
void neopixel_update(const uint8_t *buffer, const size_t len);
void neopixel_show_layer(const uint32_t *colormap, const size_t len);
void neopixel_on_layer_state_change(const uint8_t state);
