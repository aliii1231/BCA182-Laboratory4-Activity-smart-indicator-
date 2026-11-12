#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

static void SystemClock_Config(void);
static void Error_Handler(void);
static void vBringupTask(void *argument);

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    /* Safe skeleton: board I/O is withheld until the RT-Spark pinout is verified. */
    if (xTaskCreate(vBringupTask, "bringup", 256, NULL, 1, NULL) != pdPASS) {
        Error_Handler();
    }
    vTaskStartScheduler();
    Error_Handler();
}

static void vBringupTask(void *argument)
{
    (void)argument;
    for (;;) {
        /* Placeholder for diagnostic task startup once clocks and UART pins are verified. */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void SystemClock_Config(void)
{
    /* HSE source/frequency and PLL settings require board documentation confirmation. */
}

static void Error_Handler(void)
{
    __disable_irq();
    for (;;) { }
}
