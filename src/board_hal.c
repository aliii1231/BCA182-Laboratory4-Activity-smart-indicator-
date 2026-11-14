#include "app.h"
#include "board_pins.h"

I2C_HandleTypeDef hi2c2;
UART_HandleTypeDef huart1;

static void GPIO_Init_All(void)
{
    GPIO_InitTypeDef g = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE(); __HAL_RCC_GPIOB_CLK_ENABLE(); __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE(); __HAL_RCC_GPIOE_CLK_ENABLE(); __HAL_RCC_GPIOF_CLK_ENABLE(); __HAL_RCC_GPIOG_CLK_ENABLE();

    g.Mode = GPIO_MODE_OUTPUT_OD; g.Pull = GPIO_PULLUP; g.Speed = GPIO_SPEED_FREQ_LOW;
    g.Pin = AHT_SCL_PIN; HAL_GPIO_Init(AHT_GPIO, &g);
    g.Pin = AHT_SDA_PIN; HAL_GPIO_Init(AHT_GPIO, &g);
    HAL_GPIO_WritePin(AHT_GPIO, AHT_SCL_PIN | AHT_SDA_PIN, GPIO_PIN_SET);

    g.Mode = GPIO_MODE_OUTPUT_PP; g.Pull = GPIO_NOPULL;
    g.Pin = BUTTON_UP_PIN | BUTTON_DOWN_PIN | BUTTON_LEFT_PIN | BUTTON_RIGHT_PIN;
    g.Mode = GPIO_MODE_INPUT; g.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &g);

    g.Mode = GPIO_MODE_OUTPUT_PP; g.Pull = GPIO_NOPULL;
    g.Pin = LED_RED_PIN | LED_BLUE_PIN; HAL_GPIO_Init(GPIOF, &g);
    g.Pin = BUZZER_PIN; HAL_GPIO_Init(GPIOB, &g);
    g.Pin = MATRIX_ENABLE_PIN; HAL_GPIO_Init(MATRIX_ENABLE_PORT, &g);
    g.Pin = MATRIX_DATA_PIN; HAL_GPIO_Init(MATRIX_DATA_PORT, &g);
    g.Pin = LCD_RESET_PIN; HAL_GPIO_Init(LCD_RESET_PORT, &g);
    g.Pin = LCD_BACKLIGHT_PIN; HAL_GPIO_Init(LCD_BACKLIGHT_PORT, &g);
    g.Pin = LCD_TE_PIN; g.Mode = GPIO_MODE_INPUT; HAL_GPIO_Init(LCD_TE_PORT, &g);
}

static void I2C2_Init(void)
{
    GPIO_InitTypeDef g = {0};
    __HAL_RCC_GPIOF_CLK_ENABLE(); __HAL_RCC_I2C2_CLK_ENABLE();
    g.Pin = SENSOR_I2C_SCL_PIN | SENSOR_I2C_SDA_PIN; g.Mode = GPIO_MODE_AF_OD;
    g.Pull = GPIO_PULLUP; g.Speed = GPIO_SPEED_FREQ_VERY_HIGH; g.Alternate = GPIO_AF4_I2C2;
    HAL_GPIO_Init(GPIOF, &g);
    hi2c2.Instance = I2C2; hi2c2.Init.ClockSpeed = 100000; hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c2.Init.OwnAddress1 = 0; hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE; hi2c2.Init.OwnAddress2 = 0;
    hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE; hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c2) != HAL_OK) Error_Handler();
}

static void UART1_Init(void)
{
    GPIO_InitTypeDef g = {0}; __HAL_RCC_GPIOA_CLK_ENABLE(); __HAL_RCC_USART1_CLK_ENABLE();
    g.Pin = GPIO_PIN_9 | GPIO_PIN_10; g.Mode = GPIO_MODE_AF_PP; g.Pull = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_VERY_HIGH; g.Alternate = GPIO_AF7_USART1; HAL_GPIO_Init(GPIOA, &g);
    huart1.Instance = USART1; huart1.Init.BaudRate = 115200; huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1; huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX; huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) Error_Handler();
}

void Board_Peripherals_Init(void) { GPIO_Init_All(); I2C2_Init(); UART1_Init(); }

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0}; RCC_ClkInitTypeDef clk = {0};
    __HAL_RCC_PWR_CLK_ENABLE(); __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE; osc.HSEState = RCC_HSE_ON;
    osc.PLL.PLLState = RCC_PLL_ON; osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM = 8; osc.PLL.PLLN = 336; osc.PLL.PLLP = RCC_PLLP_DIV2; osc.PLL.PLLQ = 7;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) Error_Handler();
    clk.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK; clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV4; clk.APB2CLKDivider = RCC_HCLK_DIV2;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_5) != HAL_OK) Error_Handler();
}

void HAL_MspInit(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE(); __HAL_RCC_PWR_CLK_ENABLE();
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
}

static TIM_HandleTypeDef htim6_hal;

HAL_StatusTypeDef HAL_InitTick(uint32_t priority)
{
    RCC_ClkInitTypeDef clk = {0}; uint32_t latency = 0;
    HAL_RCC_GetClockConfig(&clk, &latency);
    uint32_t timer_clock = HAL_RCC_GetPCLK1Freq();
    if (clk.APB1CLKDivider != RCC_HCLK_DIV1) timer_clock *= 2U;
    __HAL_RCC_TIM6_CLK_ENABLE();
    htim6_hal.Instance = TIM6;
    htim6_hal.Init.Prescaler = (timer_clock / 1000000U) - 1U;
    htim6_hal.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim6_hal.Init.Period = 999U;
    htim6_hal.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim6_hal) != HAL_OK) return HAL_ERROR;
    HAL_NVIC_SetPriority(TIM6_DAC_IRQn, priority, 0U);
    HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
    return HAL_TIM_Base_Start_IT(&htim6_hal);
}

void TIM6_DAC_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim6_hal);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6) HAL_IncTick();
}
