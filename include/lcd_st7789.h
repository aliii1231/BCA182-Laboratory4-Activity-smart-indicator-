#ifndef LCD_ST7789_H
#define LCD_ST7789_H
#include <stdint.h>
int lcd_init(void);
void lcd_fill(uint16_t rgb565);
void lcd_draw_string(uint16_t x,uint16_t y,const char *str,uint16_t fg,uint16_t bg);
void lcd_status(uint8_t state,int32_t temp_cx100,int32_t rh_cx100,uint16_t ambient);
void lcd_test(void);
#endif
