// NeoPixel (Addressable LED) Functions for CH551, CH552 and CH554
// CC-BY-SA-3.0
// Version 1.2 by Stefan Wagner (https://github.com/wagiminator)
// Version 1.3 adapted for FAK firmware


#include "neopixel.h"
#include "keymap.h"
#include <string.h>

#define NEOPIN PIN_asm(PIN_NEO) // convert PIN_NEO for inline assembly
#define min(a,b) ((a) < (b) ? (a) : (b))

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
void neopixel_update(const uint8_t *buffer, const size_t len) {
  EA = 0;
  for(size_t i = 0; i < len; ++i) {
    neopixel_sendByte(buffer[i]);
  }
  EA = 1;
}

void neopixel_show_layer(const uint32_t *colormap, const size_t len) {
  EA = 0;
  for(size_t i = 0; i < len; ++i) {
    #ifdef NEOPIXEL_GRB
    neopixel_sendByte((colormap[i] >>  8) & 0xFF);
    neopixel_sendByte((colormap[i] >> 16) & 0xFF);
    neopixel_sendByte((colormap[i] >>  0) & 0xFF);
    #elif  NEOPIXEL_RGB
    neopixel_sendByte((colormap[i] >> 16) & 0xFF);
    neopixel_sendByte((colormap[i] >>  8) & 0xFF);
    neopixel_sendByte((colormap[i] >>  0) & 0xFF);
    #endif
  }
  EA = 1; 
}

extern __code uint32_t led_map[LAYER_COUNT][LED_COUNT];
__xdata uint8_t buffer[LED_COUNT * 3];  // pixel buffer
void neopixel_on_layer_state_change(const fak_layer_state_t state) {
  memset(buffer, 0, sizeof(buffer));

  for (unsigned int i = 0; i < LAYER_COUNT; i++) {
    if (is_layer_on(i)) {
      for (unsigned int j = 0; j < LED_COUNT; j++) {
        const uint32_t color = led_map[i][j];
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t g = (color >>  8) & 0xFF;
        uint8_t b = (color >>  0) & 0xFF;
        uint8_t a = (color >> 24) & 0xFF;  // Not used in blending here, but it's available

        // Add the color to the array with additive blending
        #ifdef NEOPIXEL_GRB
        buffer[j * 3 + 0] = min(255, buffer[j * 3 + 0] + g);
        buffer[j * 3 + 1] = min(255, buffer[j * 3 + 1] + r);
        buffer[j * 3 + 2] = min(255, buffer[j * 3 + 2] + b);
        #elif  NEOPIXEL_RGB
        buffer[j * 3 + 0] = min(255, buffer[j * 3 + 0] + r);
        buffer[j * 3 + 1] = min(255, buffer[j * 3 + 1] + g);
        buffer[j * 3 + 2] = min(255, buffer[j * 3 + 2] + b);
        #endif
      }
    }
  }
  neopixel_update(buffer, sizeof(buffer));
}

uint32_t rgba_interp(uint32_t src, uint32_t dst, uint32_t t) {
    const uint32_t s = 255 - t;
    return (
        (((((src >> 0)  & 0xff) * s +
           ((dst >> 0)  & 0xff) * t) >> 8)) |
        (((((src >> 8)  & 0xff) * s +
           ((dst >> 8)  & 0xff) * t)     )  & ~0xff) |
        (((((src >> 16) & 0xff) * s +
           ((dst >> 16) & 0xff) * t) << 8)  & ~0xffff) |
        (((((src >> 24) & 0xff) * s +
           ((dst >> 24) & 0xff) * t) << 16) & ~0xffffff)
    );
}
