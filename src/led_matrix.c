#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "board_pins.h"
#include "led_matrix.h"

#define MAX_BRIGHTNESS 32U
#define CYCLES_PER_US 168U
static uint8_t frame[MATRIX_LED_COUNT][3]; /* RGB logical order */

static inline void wait_cycles(uint32_t n)
{
    uint32_t start=DWT->CYCCNT;
    while((uint32_t)(DWT->CYCCNT-start)<n) { __NOP(); }
}

static inline void send_byte(uint8_t value)
{
    for(uint8_t mask=0x80U;mask;mask>>=1){
        HAL_GPIO_WritePin(MATRIX_DATA_PORT,MATRIX_DATA_PIN,GPIO_PIN_SET);
        if(value&mask){wait_cycles(100U);HAL_GPIO_WritePin(MATRIX_DATA_PORT,MATRIX_DATA_PIN,GPIO_PIN_RESET);wait_cycles(110U);}
        else {wait_cycles(50U);HAL_GPIO_WritePin(MATRIX_DATA_PORT,MATRIX_DATA_PIN,GPIO_PIN_RESET);wait_cycles(160U);}
    }
}

static uint8_t limit(uint8_t value)
{
    return (uint8_t)(((uint16_t)value*MAX_BRIGHTNESS+127U)/255U);
}

void matrix_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE(); __HAL_RCC_GPIOF_CLK_ENABLE();
    GPIO_InitTypeDef gpio={0}; gpio.Pin=MATRIX_DATA_PIN; gpio.Mode=GPIO_MODE_OUTPUT_PP;
    gpio.Pull=GPIO_NOPULL; gpio.Speed=GPIO_SPEED_FREQ_VERY_HIGH; HAL_GPIO_Init(MATRIX_DATA_PORT,&gpio);
    gpio.Pin=MATRIX_ENABLE_PIN; gpio.Mode=GPIO_MODE_OUTPUT_PP; gpio.Speed=GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(MATRIX_ENABLE_PORT,&gpio);
    HAL_GPIO_WritePin(MATRIX_DATA_PORT,MATRIX_DATA_PIN,GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MATRIX_ENABLE_PORT,MATRIX_ENABLE_PIN,GPIO_PIN_RESET); /* active-low /OE */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT=0U; DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    matrix_clear();
}

void matrix_clear(void)
{
    matrix_fill(0,0,0);
}

void matrix_fill(uint8_t r,uint8_t g,uint8_t b)
{
    for(uint8_t i=0;i<MATRIX_LED_COUNT;i++) matrix_set(i,r,g,b);
}

void matrix_set(uint8_t i,uint8_t r,uint8_t g,uint8_t b)
{
    if(i>=MATRIX_LED_COUNT)return;
    frame[i][0]=limit(r); frame[i][1]=limit(g); frame[i][2]=limit(b);
}

void matrix_show(void)
{
    taskENTER_CRITICAL();
    for(uint8_t i=0;i<MATRIX_LED_COUNT;i++){
        send_byte(frame[i][1]); send_byte(frame[i][0]); send_byte(frame[i][2]);
    }
    HAL_GPIO_WritePin(MATRIX_DATA_PORT,MATRIX_DATA_PIN,GPIO_PIN_RESET);
    taskEXIT_CRITICAL();
    vTaskDelay(pdMS_TO_TICKS(1)); /* >=100 us latch */
}

void matrix_test(void)
{
    matrix_fill(255,0,0); matrix_show(); vTaskDelay(pdMS_TO_TICKS(100));
    matrix_fill(0,255,0); matrix_show(); vTaskDelay(pdMS_TO_TICKS(100));
    matrix_fill(0,0,255); matrix_show(); vTaskDelay(pdMS_TO_TICKS(100));
    matrix_clear();
    for(uint8_t i=0;i<MATRIX_LED_COUNT;i++){
        matrix_clear(); matrix_set(i,255,255,255); matrix_show(); vTaskDelay(pdMS_TO_TICKS(30));
    }
    matrix_clear(); matrix_show();
}
