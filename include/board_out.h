#ifndef BOARD_OUT_H
#define BOARD_OUT_H

#include <stdint.h>

typedef enum { LED_COLOR_RED=0, LED_COLOR_BLUE=1 } led_color_t;
void Board_Out_Init(void);
void led_set(led_color_t color, uint8_t on);
void buzzer_tone(uint8_t on);
void buzzer_beep(uint32_t ms);
void Board_SelfTest(void);

#endif
