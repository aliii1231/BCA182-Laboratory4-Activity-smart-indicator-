#ifndef APP_H
#define APP_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

typedef struct {
    uint32_t tick_ms;
    float temperature_c;
    float humidity_pct;
    uint16_t ambient_raw;
    uint16_t proximity_raw;
    uint8_t valid_mask;
    uint8_t button_mask;
} sensor_sample_t;

void App_Init(void);
void App_Printf(const char *format, ...);
HAL_StatusTypeDef AHT_Read(float *temperature_c, float *humidity_pct);
HAL_StatusTypeDef AP3216C_Read(uint16_t *ambient, uint16_t *proximity);
HAL_StatusTypeDef ICM20608_ReadWhoAmI(uint8_t *whoami);
void Board_UI_Init(void);
void Board_UI_Indicate(uint8_t state);
void Board_LCD_Init(void);
void Board_LCD_Status(const char *line1, const char *line2);
void Board_Peripherals_Init(void);

#endif
