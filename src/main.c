#include "app.h"
#include "board_pins.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern I2C_HandleTypeDef hi2c2;
extern UART_HandleTypeDef huart1;
void SystemClock_Config(void);

static uint8_t data_logger_mode=0;
static char log_session[24]="unset";
static uint8_t log_label=255;
static void SensorTask(void *arg);
static void UITask(void *arg);
static void ConsoleTask(void *arg);

void App_Printf(const char *format,...)
{
    char buffer[256]; va_list args; va_start(args,format); int n=vsnprintf(buffer,sizeof buffer,format,args); va_end(args);
    if(n>0) HAL_UART_Transmit(&huart1,(uint8_t*)buffer,(uint16_t)((n<(int)sizeof buffer)?n:(int)sizeof buffer-1),1000);
}

static uint8_t buttons(void)
{
    uint8_t b=0;
    if(HAL_GPIO_ReadPin(BUTTON_UP_PORT,BUTTON_UP_PIN)==GPIO_PIN_RESET)b|=1;
    if(HAL_GPIO_ReadPin(BUTTON_DOWN_PORT,BUTTON_DOWN_PIN)==GPIO_PIN_RESET)b|=2;
    if(HAL_GPIO_ReadPin(BUTTON_LEFT_PORT,BUTTON_LEFT_PIN)==GPIO_PIN_RESET)b|=4;
    if(HAL_GPIO_ReadPin(BUTTON_RIGHT_PORT,BUTTON_RIGHT_PIN)==GPIO_PIN_RESET)b|=8;
    return b;
}

static void SensorTask(void *arg)
{
    (void)arg; TickType_t last=xTaskGetTickCount(); uint32_t seq=0;
    App_Printf("BOOT,RT-SPARK,STM32F407ZGT6,clock_hz=%lu\r\n",(unsigned long)HAL_RCC_GetHCLKFreq());
    App_Printf("DIAG,interval_ms=1000,cal_temp_c=0.0,cal_rh_pct=0.0\r\n");
    if(data_logger_mode) App_Printf("CSV,session_id,label,sequence,tick_ms,temp_c,rh_pct,ambient_raw,proximity_raw,valid_mask,buttons\r\n");
    for(;;){
        sensor_sample_t s={0}; s.tick_ms=HAL_GetTick(); s.button_mask=buttons();
        float t=0,h=0; uint16_t a=0,p=0; uint8_t w=0;
        if(AHT_Read(&t,&h)==HAL_OK){s.temperature_c=t;s.humidity_pct=h;s.valid_mask|=1;}
        if(AP3216C_Read(&a,&p)==HAL_OK){s.ambient_raw=a;s.proximity_raw=p;s.valid_mask|=2;}
        HAL_StatusTypeDef icm=ICM20608_ReadWhoAmI(&w);
        if(data_logger_mode){
          App_Printf("DATA,%s,%u,%lu,%lu,%.3f,%.3f,%u,%u,%u,%u\r\n",log_session,(unsigned)log_label,(unsigned long)seq++,(unsigned long)s.tick_ms,s.temperature_c,s.humidity_pct,s.ambient_raw,s.proximity_raw,s.valid_mask,s.button_mask);
        } else {
          App_Printf("SENSOR,tick_ms=%lu,temp_c=%.2f,rh_pct=%.2f,ambient_raw=%u,proximity_raw=%u,valid=0x%02X,icm_whoami=0x%02X,icm_ok=%u,interval_ms=1000,cal_temp_c=0,cal_rh_pct=0\r\n",(unsigned long)s.tick_ms,s.temperature_c,s.humidity_pct,s.ambient_raw,s.proximity_raw,s.valid_mask,w,(icm==HAL_OK));
        }
        vTaskDelayUntil(&last,pdMS_TO_TICKS(1000));
    }
}

static void UITask(void *arg)
{
    (void)arg; uint8_t prev=0;
    for(;;){uint8_t b=buttons(); if(b && b!=prev) App_Printf("BUTTON,tick_ms=%lu,mask=%u\r\n",(unsigned long)HAL_GetTick(),b); prev=b; vTaskDelay(pdMS_TO_TICKS(25));}
}

static void ConsoleTask(void *arg)
{
    (void)arg; uint8_t ch; char line[40]; unsigned used=0;
    App_Printf("COMMANDS: d=diagnostic, l=<session>,<label 0-3> logger, x=stop logger\r\n");
    for(;;){
      if(HAL_UART_Receive(&huart1,&ch,1,10)==HAL_OK){
        if(ch=='\r'||ch=='\n'){
          line[used]=0;
          if(line[0]=='l'&&line[1]=='='){
            char *comma=strchr(line+2,',');
            if(comma){*comma=0; unsigned lab=(unsigned)atoi(comma+1); size_t n=strlen(line+2);
              if(n>0&&n<sizeof(log_session)&&lab<=3){memcpy(log_session,line+2,n+1);log_label=(uint8_t)lab;data_logger_mode=1;App_Printf("MODE,logger,session=%s,label=%u\r\n",log_session,lab);}
              else App_Printf("ERROR,logger_command\r\n");
            } else App_Printf("ERROR,expected_l=session,label\r\n");
          } else if(line[0]=='d'){data_logger_mode=0;App_Printf("MODE,diagnostic\r\n");}
          else if(line[0]=='x'){data_logger_mode=0;App_Printf("MODE,stopped\r\n");}
          used=0;
        } else if(used<sizeof(line)-1) line[used++]=(char)ch;
      }
      vTaskDelay(pdMS_TO_TICKS(10));
    }
}

int main(void)
{
    HAL_Init(); SystemClock_Config(); Board_Peripherals_Init(); Board_UI_Init(); Board_LCD_Init();
    App_Printf("INIT,uart=ok,i2c2=%s,aht_soft_i2c=ready,lcd=reset_only\r\n",HAL_I2C_GetState(&hi2c2)==HAL_I2C_STATE_READY?"ready":"error");
    if(xTaskCreate(SensorTask,"sensor",512,NULL,3,NULL)!=pdPASS || xTaskCreate(UITask,"ui",256,NULL,2,NULL)!=pdPASS || xTaskCreate(ConsoleTask,"console",384,NULL,2,NULL)!=pdPASS) Error_Handler();
    vTaskStartScheduler(); Error_Handler();
}

void Error_Handler(void){__disable_irq();for(;;){} }
