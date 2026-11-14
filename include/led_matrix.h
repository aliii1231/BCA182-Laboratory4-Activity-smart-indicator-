#ifndef LED_MATRIX_H
#define LED_MATRIX_H

#include <stdint.h>

#define MATRIX_LED_COUNT 19U
void matrix_init(void);
void matrix_clear(void);
void matrix_fill(uint8_t r, uint8_t g, uint8_t b);
void matrix_set(uint8_t i, uint8_t r, uint8_t g, uint8_t b);
void matrix_show(void);
void matrix_test(void);

#endif
