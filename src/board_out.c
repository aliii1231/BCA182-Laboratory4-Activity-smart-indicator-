#include "app.h"
#include "board_pins.h"
#include "board_out.h"
#include "led_matrix.h"
#include "lcd_st7789.h"

extern void App_Printf(const char *format, ...);
static TIM_HandleTypeDef htim3;

void Board_Out_Init(void)
{
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef gpio={0};
    gpio.Pin=BUZZER_PIN; gpio.Mode=GPIO_MODE_AF_PP; gpio.Pull=GPIO_NOPULL;
    gpio.Speed=GPIO_SPEED_FREQ_LOW; gpio.Alternate=GPIO_AF2_TIM3;
    HAL_GPIO_Init(BUZZER_PORT,&gpio);
    htim3.Instance=TIM3;
    htim3.Init.Prescaler=(HAL_RCC_GetPCLK1Freq()*2U/1000000U)-1U;
    htim3.Init.CounterMode=TIM_COUNTERMODE_UP;
    htim3.Init.Period=369U;
    htim3.Init.ClockDivision=TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload=TIM_AUTORELOAD_PRELOAD_DISABLE;
    if(HAL_TIM_PWM_Init(&htim3)!=HAL_OK) Error_Handler();
    TIM_OC_InitTypeDef oc={0}; oc.OCMode=TIM_OCMODE_PWM1; oc.Pulse=185U;
    oc.OCPolarity=TIM_OCPOLARITY_HIGH; oc.OCFastMode=TIM_OCFAST_DISABLE;
    if(HAL_TIM_PWM_ConfigChannel(&htim3,&oc,TIM_CHANNEL_3)!=HAL_OK) Error_Handler();
    HAL_TIM_PWM_Start(&htim3,TIM_CHANNEL_3);
    buzzer_tone(0);
    led_set(LED_COLOR_RED,0); led_set(LED_COLOR_BLUE,0);
}

void led_set(led_color_t color, uint8_t on)
{
    if(color==LED_COLOR_RED) HAL_GPIO_WritePin(LED_RED_PORT,LED_RED_PIN,on?GPIO_PIN_RESET:GPIO_PIN_SET);
    else if(color==LED_COLOR_BLUE) HAL_GPIO_WritePin(LED_BLUE_PORT,LED_BLUE_PIN,on?GPIO_PIN_RESET:GPIO_PIN_SET);
}

void buzzer_tone(uint8_t on)
{
    __HAL_TIM_SET_COMPARE(&htim3,TIM_CHANNEL_3,on?185U:0U);
}

void buzzer_beep(uint32_t ms)
{
    TickType_t start=xTaskGetTickCount(); buzzer_tone(1);
    while((uint32_t)(xTaskGetTickCount()-start)<pdMS_TO_TICKS(ms)) vTaskDelay(pdMS_TO_TICKS(1));
    buzzer_tone(0);
}

void Board_SelfTest(void)
{
    App_Printf("SELFTEST,tick_ms=%lu,step=red_on\r\n",(unsigned long)HAL_GetTick());
    led_set(LED_COLOR_RED,1); vTaskDelay(pdMS_TO_TICKS(20)); led_set(LED_COLOR_RED,0);
    App_Printf("SELFTEST,tick_ms=%lu,step=blue_on\r\n",(unsigned long)HAL_GetTick());
    led_set(LED_COLOR_BLUE,1); vTaskDelay(pdMS_TO_TICKS(20)); led_set(LED_COLOR_BLUE,0);
    App_Printf("SELFTEST,tick_ms=%lu,step=beep\r\n",(unsigned long)HAL_GetTick());
    buzzer_tone(1); vTaskDelay(pdMS_TO_TICKS(20)); buzzer_tone(0);
    App_Printf("SELFTEST,tick_ms=%lu,step=matrix\r\n",(unsigned long)HAL_GetTick());
    matrix_test();
    App_Printf("SELFTEST,tick_ms=%lu,step=lcd\r\n",(unsigned long)HAL_GetTick());
    lcd_test();
}
