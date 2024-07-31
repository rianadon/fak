// NeoPixel (Addressable LED) Functions for CH551, CH552 and CH554
// CC-BY-SA-3.0
// Version 1.2 by Stefan Wagner (https://github.com/wagiminator)
// Version 1.3 adapted for FAK firmware


#pragma once
#include <stdint.h>

// typedef struct {
//   sbit pin;
//   uint8_t  count;
//   uint24_t *buffer;
// } neopixel_def_t;

// #define neopixel_latch() DLY_us(281) // latch colors

void neopixel_sendByte(uint8_t data); // send a single byte to the pixels
void neopixel_clearAll(void);         // clear all pixels
void neopixel_update(void);           // write buffer to pixels
void neopixel_writeColor(uint8_t pixel, uint8_t r, uint8_t g,
                    uint8_t b); // write color to pixel in buffer
void neopixel_writeHue(uint8_t pixel, uint8_t hue,
                  uint8_t bright);  // hue (0..191), brightness (0..2)
void neopixel_clearPixel(uint8_t pixel); // clear one pixel in buffer
