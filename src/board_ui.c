#include "app.h"
#include "board_pins.h"

void Board_UI_Init(void)
{
    HAL_GPIO_WritePin(LED_RED_PORT,LED_RED_PIN,GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_BLUE_PORT,LED_BLUE_PIN,GPIO_PIN_SET);
    HAL_GPIO_WritePin(BUZZER_PORT,BUZZER_PIN,GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MATRIX_ENABLE_PORT,MATRIX_ENABLE_PIN,GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_BACKLIGHT_PORT,LCD_BACKLIGHT_PIN,GPIO_PIN_SET);
}

void Board_UI_Indicate(uint8_t state)
{
    HAL_GPIO_WritePin(LED_RED_PORT,LED_RED_PIN,(state==2||state==3)?GPIO_PIN_RESET:GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_BLUE_PORT,LED_BLUE_PIN,(state==1)?GPIO_PIN_RESET:GPIO_PIN_SET);
    HAL_GPIO_WritePin(BUZZER_PORT,BUZZER_PIN,(state==2||state==3)?GPIO_PIN_SET:GPIO_PIN_RESET);
}

void Board_LCD_Init(void)
{
    /* ST7789V3 is on the FSMC 16-bit bus. Keep reset/backlight safe here;
       controller command sequence and graphics driver are intentionally bounded. */
    HAL_GPIO_WritePin(LCD_RESET_PORT,LCD_RESET_PIN,GPIO_PIN_RESET);
    HAL_Delay(5); HAL_GPIO_WritePin(LCD_RESET_PORT,LCD_RESET_PIN,GPIO_PIN_SET); HAL_Delay(120);
    HAL_GPIO_WritePin(LCD_BACKLIGHT_PORT,LCD_BACKLIGHT_PIN,GPIO_PIN_SET);
}

void Board_LCD_Status(const char *line1,const char *line2)
{
    /* The 240x240 ST7789 FSMC graphics path is not implemented in this scaffold.
       Keep status over UART and LED feedback; do not pretend text was displayed. */
    (void)line1; (void)line2;
}
