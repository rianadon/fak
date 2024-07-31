// NeoPixel (Addressable LED) Functions for CH551, CH552 and CH554
// CC-BY-SA-3.0
// Version 1.2 by Stefan Wagner (https://github.com/wagiminator)
// Version 1.3 adapted for FAK firmware


#include "neopixel.h"

#define NEOPIN PIN_asm(PIN_NEO) // convert PIN_NEO for inline assembly
__xdata uint8_t neopixel_buffer[3 * NEOPIXEL_COUNT]; // pixel buffer
__xdata uint8_t *ptr;                      // pixel buffer pointer

// ===================================================================================
// Protocol Delays
// ===================================================================================
// There are three essential conditions:
// - T0H (HIGH-time for "0"-bit) must be max.  500ns
// - T1H (HIGH-time for "1"-bit) must be min.  625ns
// - TCT (total clock time) must be      min. 1150ns
// The bit transmission loop takes 11 clock cycles.
#if F_CPU == 24000000       // 24 MHz system clock
  #define T1H_DELAY \
    nop             \
    nop             \
    nop             \
    nop             \
    nop             \
    nop             \
    nop             \
    nop             \
    nop             \
    nop             \
    nop                     // 15 - 4 = 11 clock cycles for min 625ns
  #define TCT_DELAY \
    nop             \
    nop             \
    nop             \
    nop             \
    nop             \
    nop                     // 28 - 11 - 11 = 6 clock cycles for min 1150ns
#elif F_CPU == 16000000     // 16 MHz system clock
  #define T1H_DELAY \
    nop             \
    nop             \
    nop             \
    nop             \
    nop             \
    nop                     // 10 - 4 = 6 clock cycles for min 625ns
  #define TCT_DELAY \
    nop             \
    nop                     // 19 - 6 - 11 = 2 clock cycles for min 1150ns
#elif F_CPU == 12000000     // 12 MHz system clock
  #define T1H_DELAY \
    nop             \
    nop             \
    nop             \
    nop                     // 8 - 4 = 4 clock cycles for min 625ns
  #define TCT_DELAY         // 14 - 4 - 11 < 0 clock cycles for min 1150ns
#elif F_CPU == 6000000      // 13 MHz system clock
  #define T1H_DELAY         // 4 - 4 = 0 clock cycles for min 625ns
  #define TCT_DELAY         // 7 - 0 - 11 < 0 clock cycles for min 1150ns
#else
  #error Unsupported system clock frequency for NeoPixels!
#endif

// ===================================================================================
// Send a Data Byte to the Pixels String
// ===================================================================================
// This is the most time sensitive part. Outside of the function, it must be
// ensured that interrupts are disabled and that the time between the
// transmission of the individual bytes is less than the pixel's latch time.
void neopixel_sendByte(uint8_t data) {
  data;                 // stop unreferenced argument warning
  __asm
    .even
    mov  r7, #8          ; 2 CLK - 8 bits to transfer
    xch  a, dpl          ; 2 CLK - data byte -> accu
    01$:
    rlc  a               ; 1 CLK - data bit -> carry (MSB first)
    setb NEOPIXEL_PIN    ; 2 CLK - NEO pin HIGH
    mov  NEOPIXEL_PIN, c ; 2 CLK - "0"-bit? -> NEO pin LOW now
    T1H_DELAY            ; x CLK - TH1 delay
    clr  NEOPIXEL_PIN    ; 2 CLK - "1"-bit? -> NEO pin LOW a little later
    TCT_DELAY            ; y CLK - TCT delay
    djnz r7, 01$         ; 2/4|5|6 CLK - repeat for all bits
  __endasm;
}

// ===================================================================================
// Write Buffer to Pixels
// ===================================================================================
void neopixel_update(void) {
  uint8_t i;
  ptr = neopixel_buffer;
  EA = 0;
  for (i = 3 * NEOPIXEL_COUNT; i; i--)
    neopixel_sendByte(*ptr++);
  EA = 1;
}

// ===================================================================================
// Clear all Pixels
// ===================================================================================
void neopixel_clearAll(void) {
  uint8_t i;
  ptr = neopixel_buffer;
  for (i = 3 * NEOPIXEL_COUNT; i; i--)
    *ptr++ = 0;
  neopixel_update();
}

// ===================================================================================
// Write Color to a Single Pixel in Buffer
// ===================================================================================
void neopixel_writeColor(uint8_t pixel, uint8_t r, uint8_t g, uint8_t b) {
  ptr = neopixel_buffer + (3 * pixel);
#if defined(NEOPIXEL_GRB)
  *ptr++ = g;
  *ptr++ = r;
  *ptr = b;
#elif defined(NEOPIXEL_RGB)
  *ptr++ = r;
  *ptr++ = g;
  *ptr = b;
#else
#error Wrong or missing NeoPixel type definition!
#endif
}

// ===================================================================================
// Write Hue Value (0..191) and Brightness (0..2) to a Single Pixel in Buffer
// ===================================================================================
void neopixel_writeHue(uint8_t pixel, uint8_t hue, uint8_t bright) {
  uint8_t phase = hue >> 6;
  uint8_t step = (hue & 63) << bright;
  uint8_t nstep = (63 << bright) - step;
  switch (phase) {
  case 0:
    neopixel_writeColor(pixel, nstep, step, 0);
    break;
  case 1:
    neopixel_writeColor(pixel, 0, nstep, step);
    break;
  case 2:
    neopixel_writeColor(pixel, step, 0, nstep);
    break;
  default:
    break;
  }
}

// ===================================================================================
// Clear Single Pixel in Buffer
// ===================================================================================
void neopixel_clearPixel(uint8_t pixel) { neopixel_writeColor(pixel, 0, 0, 0); }
