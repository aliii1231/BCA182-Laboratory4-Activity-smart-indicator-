#include "app.h"
#include "board_pins.h"
#include "board_out.h"
#include "led_matrix.h"
#include "lcd_st7789.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semphr.h"

extern I2C_HandleTypeDef hi2c2;
extern UART_HandleTypeDef huart1;
void SystemClock_Config(void);
static RTC_HandleTypeDef hrtc;
static uint8_t rtc_ready=0;
static SPI_HandleTypeDef hspi2_diag;
static SD_HandleTypeDef hsd_diag;

static void storage_diagnostics(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE(); __HAL_RCC_GPIOC_CLK_ENABLE(); __HAL_RCC_GPIOD_CLK_ENABLE(); __HAL_RCC_GPIOF_CLK_ENABLE(); __HAL_RCC_SPI2_CLK_ENABLE();
    GPIO_InitTypeDef g={0};
    g.Pin=GPIO_PIN_13;g.Mode=GPIO_MODE_AF_PP;g.Pull=GPIO_NOPULL;g.Speed=GPIO_SPEED_FREQ_VERY_HIGH;g.Alternate=GPIO_AF5_SPI2;HAL_GPIO_Init(GPIOB,&g);
    g.Pin=GPIO_PIN_2|GPIO_PIN_3;g.Mode=GPIO_MODE_AF_PP;g.Alternate=GPIO_AF5_SPI2;HAL_GPIO_Init(GPIOC,&g);
    g.Pin=GPIO_PIN_12;g.Mode=GPIO_MODE_OUTPUT_PP;g.Pull=GPIO_PULLUP;HAL_GPIO_Init(GPIOB,&g);HAL_GPIO_WritePin(GPIOB,GPIO_PIN_12,GPIO_PIN_SET);
    g.Pin=WIFI_CS_PIN;g.Mode=GPIO_MODE_OUTPUT_PP;g.Pull=GPIO_NOPULL;HAL_GPIO_Init(WIFI_CS_PORT,&g);HAL_GPIO_WritePin(WIFI_CS_PORT,WIFI_CS_PIN,GPIO_PIN_SET);
    hspi2_diag.Instance=SPI2;hspi2_diag.Init.Mode=SPI_MODE_MASTER;hspi2_diag.Init.Direction=SPI_DIRECTION_2LINES;
    hspi2_diag.Init.DataSize=SPI_DATASIZE_8BIT;hspi2_diag.Init.CLKPolarity=SPI_POLARITY_LOW;hspi2_diag.Init.CLKPhase=SPI_PHASE_1EDGE;
    hspi2_diag.Init.NSS=SPI_NSS_SOFT;hspi2_diag.Init.BaudRatePrescaler=SPI_BAUDRATEPRESCALER_64;hspi2_diag.Init.FirstBit=SPI_FIRSTBIT_MSB;
    hspi2_diag.Init.TIMode=SPI_TIMODE_DISABLE;hspi2_diag.Init.CRCCalculation=SPI_CRCCALCULATION_DISABLE;hspi2_diag.Init.CRCPolynomial=7;
    uint8_t id[3]={0};uint8_t tx[4]={0x9F,0xFF,0xFF,0xFF},rx[4]={0};
    uint8_t flash_ok=0,capacity_mb=0;
    if(HAL_SPI_Init(&hspi2_diag)==HAL_OK){HAL_GPIO_WritePin(GPIOB,GPIO_PIN_12,GPIO_PIN_RESET);HAL_StatusTypeDef st=HAL_SPI_TransmitReceive(&hspi2_diag,tx,rx,4,100);HAL_GPIO_WritePin(GPIOB,GPIO_PIN_12,GPIO_PIN_SET);if(st==HAL_OK){id[0]=rx[1];id[1]=rx[2];id[2]=rx[3];if(id[0]==0xEF&&id[1]==0x40){if(id[2]==0x16){capacity_mb=4;flash_ok=1;}else if(id[2]==0x17){capacity_mb=8;flash_ok=1;}else if(id[2]==0x18){capacity_mb=16;flash_ok=1;}}}}
    App_Printf("FLASH,id=%02X%02X%02X,capacity_mb=%u,ok=%u\r\n",id[0],id[1],id[2],capacity_mb,flash_ok);

    __HAL_RCC_SDIO_CLK_ENABLE();
    g.Pin=GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12;g.Mode=GPIO_MODE_AF_PP;g.Pull=GPIO_PULLUP;g.Speed=GPIO_SPEED_FREQ_VERY_HIGH;g.Alternate=GPIO_AF12_SDIO;HAL_GPIO_Init(GPIOC,&g);
    g.Pin=GPIO_PIN_2;HAL_GPIO_Init(GPIOD,&g);
    g.Pin=GPIO_PIN_3;g.Mode=GPIO_MODE_INPUT;g.Pull=GPIO_PULLUP;HAL_GPIO_Init(GPIOF,&g);
    uint8_t present=(HAL_GPIO_ReadPin(GPIOF,GPIO_PIN_3)==GPIO_PIN_RESET);uint8_t sd_ok=0;uint32_t blocks=0;uint8_t block[512];
    if(present){
      hsd_diag.Instance=SDIO;hsd_diag.Init.ClockEdge=SDIO_CLOCK_EDGE_RISING;hsd_diag.Init.ClockBypass=SDIO_CLOCK_BYPASS_DISABLE;
      hsd_diag.Init.ClockPowerSave=SDIO_CLOCK_POWER_SAVE_DISABLE;hsd_diag.Init.BusWide=SDIO_BUS_WIDE_1B;hsd_diag.Init.HardwareFlowControl=SDIO_HARDWARE_FLOW_CONTROL_DISABLE;hsd_diag.Init.ClockDiv=118;
      if(HAL_SD_Init(&hsd_diag)==HAL_OK && HAL_SD_ConfigWideBusOperation(&hsd_diag,SDIO_BUS_WIDE_4B)==HAL_OK){HAL_SD_CardInfoTypeDef info;if(HAL_SD_GetCardInfo(&hsd_diag,&info)==HAL_OK){blocks=(uint32_t)info.BlockNbr;sd_ok=(HAL_SD_ReadBlocks(&hsd_diag,block,0,1,500)==HAL_OK);}}
    }
    App_Printf("SD,present=%u,blocks=%lu,ok=%u\r\n",present,(unsigned long)blocks,sd_ok);
}

static void rtc_init(void)
{
    __HAL_RCC_PWR_CLK_ENABLE(); HAL_PWR_EnableBkUpAccess();
    RCC_OscInitTypeDef osc={0}; osc.OscillatorType=RCC_OSCILLATORTYPE_LSE; osc.LSEState=RCC_LSE_ON;
    if(HAL_RCC_OscConfig(&osc)!=HAL_OK) return;
    RCC_PeriphCLKInitTypeDef pclk={0}; pclk.PeriphClockSelection=RCC_PERIPHCLK_RTC; pclk.RTCClockSelection=RCC_RTCCLKSOURCE_LSE;
    if(HAL_RCCEx_PeriphCLKConfig(&pclk)!=HAL_OK)return;
    __HAL_RCC_RTC_ENABLE(); hrtc.Instance=RTC; hrtc.Init.HourFormat=RTC_HOURFORMAT_24;
    hrtc.Init.AsynchPrediv=127; hrtc.Init.SynchPrediv=255; hrtc.Init.OutPut=RTC_OUTPUT_DISABLE;
    hrtc.Init.OutPutPolarity=RTC_OUTPUT_POLARITY_HIGH; hrtc.Init.OutPutType=RTC_OUTPUT_TYPE_OPENDRAIN;
    if(HAL_RTC_Init(&hrtc)!=HAL_OK)return;
    if(HAL_RTCEx_BKUPRead(&hrtc,RTC_BKP_DR0)!=0x51A7U){
        RTC_TimeTypeDef t={0}; RTC_DateTypeDef d={0};
        t.Hours=0;t.Minutes=0;t.Seconds=0;t.TimeFormat=RTC_HOURFORMAT_24;
        d.Year=26;d.Month=1;d.Date=1;d.WeekDay=RTC_WEEKDAY_THURSDAY;
        if(HAL_RTC_SetTime(&hrtc,&t,RTC_FORMAT_BIN)!=HAL_OK || HAL_RTC_SetDate(&hrtc,&d,RTC_FORMAT_BIN)!=HAL_OK)return;
        HAL_RTCEx_BKUPWrite(&hrtc,RTC_BKP_DR0,0x51A7U);
    }
    rtc_ready=1;
}

static uint8_t rtc_set_from_text(const char *s)
{
    unsigned y,mo,d,h,mi,se; char tail;
    if(sscanf(s,"%4u-%2u-%2u,%2u:%2u:%2u%c",&y,&mo,&d,&h,&mi,&se,&tail)!=6 || y<2000||y>2099||mo<1||mo>12||d<1||d>31||h>23||mi>59||se>59)return 0;
    RTC_TimeTypeDef t={0};RTC_DateTypeDef dt={0};t.Hours=h;t.Minutes=mi;t.Seconds=se;t.TimeFormat=RTC_HOURFORMAT_24;
    dt.Year=(uint8_t)(y-2000);dt.Month=(uint8_t)mo;dt.Date=(uint8_t)d;dt.WeekDay=RTC_WEEKDAY_MONDAY;
    if(HAL_RTC_SetTime(&hrtc,&t,RTC_FORMAT_BIN)!=HAL_OK||HAL_RTC_SetDate(&hrtc,&dt,RTC_FORMAT_BIN)!=HAL_OK)return 0;
    HAL_RTCEx_BKUPWrite(&hrtc,RTC_BKP_DR0,0x51A7U);return 1;
}

static void rtc_print(void)
{
    if(!rtc_ready)return;
    RTC_TimeTypeDef t={0};RTC_DateTypeDef d={0};
    if(HAL_RTC_GetTime(&hrtc,&t,RTC_FORMAT_BIN)!=HAL_OK||HAL_RTC_GetDate(&hrtc,&d,RTC_FORMAT_BIN)!=HAL_OK){App_Printf("RTC,iso=NA\r\n");return;}
    App_Printf("RTC,iso=20%02u-%02u-%02uT%02u:%02u:%02u\r\n",d.Year,d.Month,d.Date,t.Hours,t.Minutes,t.Seconds);
}

static volatile uint8_t data_logger_mode=0;
static SemaphoreHandle_t uart_mutex;
static char log_session[24]="unset";
static volatile uint8_t log_label=255;
static void SensorTask(void *arg);
static void UITask(void *arg);
static void ConsoleTask(void *arg);

static void fx(char *out, size_t out_size, float value, unsigned decimals)
{
    int32_t scale = (decimals == 3U) ? 1000 : 100;
    int32_t scaled = (int32_t)(value * (float)scale + (value >= 0.0f ? 0.5f : -0.5f));
    uint32_t magnitude = (scaled < 0) ? (uint32_t)(-scaled) : (uint32_t)scaled;
    uint32_t whole = magnitude / (uint32_t)scale;
    uint32_t fraction = magnitude % (uint32_t)scale;
    if (decimals == 3U) snprintf(out, out_size, "%s%lu.%03lu", scaled < 0 ? "-" : "", (unsigned long)whole, (unsigned long)fraction);
    else snprintf(out, out_size, "%s%lu.%02lu", scaled < 0 ? "-" : "", (unsigned long)whole, (unsigned long)fraction);
}

void App_Printf(const char *format,...)
{
    char buffer[256];
    BaseType_t lock = (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED);
    if(lock && uart_mutex) xSemaphoreTake(uart_mutex, portMAX_DELAY);
    va_list args; va_start(args,format); int n=vsnprintf(buffer,sizeof buffer,format,args); va_end(args);
    if(n>0) {
        if(n>(int)sizeof buffer-1)n=(int)sizeof buffer-1;
        if(n<2 || buffer[n-2]!='\r' || buffer[n-1]!='\n') {
            if(n>(int)sizeof buffer-3)n=(int)sizeof buffer-3;
            buffer[n++]='\r'; buffer[n++]='\n';
        }
        HAL_StatusTypeDef tx=HAL_UART_Transmit(&huart1,(uint8_t*)buffer,(uint16_t)n,1000U);
        if(tx==HAL_BUSY) (void)HAL_UART_Transmit(&huart1,(uint8_t*)buffer,(uint16_t)n,1000U);
    }
    if(lock && uart_mutex) xSemaphoreGive(uart_mutex);
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
    TickType_t last_rtc=last;
    char ts[16], hs[16];
    uint8_t imu_ok=0;
    for (unsigned attempt=0; attempt<3 && !imu_ok; ++attempt) {
        if (ICM20608_Init()==HAL_OK) imu_ok=1;
        else if (attempt<2) vTaskDelay(pdMS_TO_TICKS(100));
    }
    App_Printf("BOOT,RT-SPARK,STM32F407ZGT6,clock_hz=%lu\r\n",(unsigned long)HAL_RCC_GetHCLKFreq());
    storage_diagnostics();
    App_Printf("DIAG,interval_ms=1000,cal_temp_c=0.0,cal_rh_pct=0.0\r\n");
    if(data_logger_mode) App_Printf("CSV,session_id,label,sequence,tick_ms,temp_c,rh_pct,ambient_raw,proximity_raw,valid_mask,buttons\r\n");
    for(;;){
        if((TickType_t)(xTaskGetTickCount()-last_rtc)>=pdMS_TO_TICKS(10000)){rtc_print();last_rtc=xTaskGetTickCount();}
        sensor_sample_t s={0}; s.tick_ms=HAL_GetTick(); s.button_mask=buttons();
        float t=0,h=0; uint16_t a=0,p=0; uint8_t w=0;
        HAL_StatusTypeDef aht_status=AHT_Read(&t,&h);
        if(aht_status==HAL_OK){fx(ts,sizeof ts,t,3);fx(hs,sizeof hs,h,3);}
        if(aht_status==HAL_OK){s.temperature_c=t;s.humidity_pct=h;s.valid_mask|=1;}
        if(AP3216C_Read(&a,&p)==HAL_OK){s.ambient_raw=a;s.proximity_raw=p;s.valid_mask|=2;}
        HAL_StatusTypeDef icm=ICM20608_ReadWhoAmI(&w);
        imu_raw_t imu;
        HAL_StatusTypeDef imu_status=(imu_ok && ICM20608_ReadRaw(&imu)==HAL_OK)?HAL_OK:HAL_ERROR;
        if(imu_status==HAL_OK) App_Printf("IMU,tick_ms=%lu,ax_mg=%ld,ay_mg=%ld,az_mg=%ld,gx_mdps=%ld,gy_mdps=%ld,gz_mdps=%ld,imu_temp_cx100=%ld,imu_ok=1\r\n",(unsigned long)HAL_GetTick(),(long)imu.ax*1000L/16384L,(long)imu.ay*1000L/16384L,(long)imu.az*1000L/16384L,(long)imu.gx*1000L/131L,(long)imu.gy*1000L/131L,(long)imu.gz*1000L/131L,(long)imu.temp*1000L/3268L+2500L);
        else App_Printf("IMU,tick_ms=%lu,imu_ok=0\r\n",(unsigned long)HAL_GetTick());
        if(data_logger_mode){
          if(aht_status==HAL_OK) App_Printf("DATA,%s,%u,%lu,%lu,%s,%s,%u,%u,%u,%u\r\n",log_session,(unsigned)log_label,(unsigned long)seq++,(unsigned long)s.tick_ms,ts,hs,s.ambient_raw,s.proximity_raw,s.valid_mask,s.button_mask);
          else App_Printf("DATA,%s,%u,%lu,%lu,NA,NA,%u,%u,%u,%u\r\n",log_session,(unsigned)log_label,(unsigned long)seq++,(unsigned long)s.tick_ms,s.ambient_raw,s.proximity_raw,s.valid_mask,s.button_mask);
        } else {
          if(aht_status==HAL_OK){fx(ts,sizeof ts,t,2);fx(hs,sizeof hs,h,2);App_Printf("SENSOR,tick_ms=%lu,temp_c=%s,rh_pct=%s,ambient_raw=%u,proximity_raw=%u,valid=0x%02X,icm_whoami=0x%02X,icm_ok=%u,interval_ms=1000,cal_temp_c=0,cal_rh_pct=0\r\n",(unsigned long)s.tick_ms,ts,hs,s.ambient_raw,s.proximity_raw,s.valid_mask,w,(icm==HAL_OK));}
          else App_Printf("SENSOR,tick_ms=%lu,temp_c=NA,rh_pct=NA,ambient_raw=%u,proximity_raw=%u,valid=0x%02X,aht_error=%u,icm_whoami=0x%02X,icm_ok=%u,interval_ms=1000,cal_temp_c=0,cal_rh_pct=0\r\n",(unsigned long)s.tick_ms,s.ambient_raw,s.proximity_raw,s.valid_mask,(unsigned)aht_status,w,(icm==HAL_OK));
        }
        vTaskDelayUntil(&last,pdMS_TO_TICKS(1000));
    }
}

static void UITask(void *arg)
{
    (void)arg; uint8_t prev=0, page=0, muted=0;
    for(;;){
        uint8_t b=buttons();
        if(b && b!=prev){
            App_Printf("BUTTON,tick_ms=%lu,mask=%u\r\n",(unsigned long)HAL_GetTick(),b);
            if((b&1U)!=0U){page++;App_Printf("UI,tick_ms=%lu,page=%u\r\n",(unsigned long)HAL_GetTick(),page);}
            if((b&2U)!=0U){if(page>0)page--;App_Printf("UI,tick_ms=%lu,page=%u\r\n",(unsigned long)HAL_GetTick(),page);}
            if((b&4U)!=0U){muted=!muted;App_Printf("UI,tick_ms=%lu,mute=%u\r\n",(unsigned long)HAL_GetTick(),muted);}
            if((b&8U)!=0U)Board_SelfTest();
        }
        prev=b; vTaskDelay(pdMS_TO_TICKS(25));
    }
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
              if(n>0&&n<sizeof(log_session)&&lab<=3){taskENTER_CRITICAL();memcpy(log_session,line+2,n+1);taskEXIT_CRITICAL();log_label=(uint8_t)lab;data_logger_mode=1;App_Printf("MODE,logger,session=%s,label=%u\r\n",log_session,lab);App_Printf("CSV,session_id,label,sequence,tick_ms,temp_c,rh_pct,ambient_raw,proximity_raw,valid_mask,buttons\r\n");}
              else App_Printf("ERROR,logger_command\r\n");
            } else App_Printf("ERROR,expected_l=session,label\r\n");
          } else if(line[0]=='t'&&line[1]=='='){if(rtc_ready&&rtc_set_from_text(line+2))App_Printf("RTC,set=ok\r\n");else App_Printf("RTC,set=error\r\n");}
          else if(line[0]=='d'){data_logger_mode=0;App_Printf("MODE,diagnostic\r\n");}
          else if(line[0]=='x'){data_logger_mode=0;App_Printf("MODE,stopped\r\n");}
          used=0;
        } else if(used<sizeof(line)-1) line[used++]=(char)ch;
      }
      vTaskDelay(pdMS_TO_TICKS(10));
    }
}

int main(void)
{
    HAL_Init(); SystemClock_Config(); Board_Peripherals_Init(); Board_UI_Init(); Board_Out_Init(); matrix_init(); rtc_init();
    if(!lcd_init()) App_Printf("INIT,lcd=error\r\n");
    App_Printf("INIT,uart=ok,i2c2=%s,aht_soft_i2c=ready,lcd=initialized\r\n",HAL_I2C_GetState(&hi2c2)==HAL_I2C_STATE_READY?"ready":"error");
    uart_mutex=xSemaphoreCreateMutex();
    if(uart_mutex==NULL || xTaskCreate(SensorTask,"sensor",512,NULL,3,NULL)!=pdPASS || xTaskCreate(UITask,"ui",384,NULL,2,NULL)!=pdPASS || xTaskCreate(ConsoleTask,"console",384,NULL,2,NULL)!=pdPASS) Error_Handler();
    vTaskStartScheduler(); Error_Handler();
}

void Error_Handler(void){__disable_irq();for(;;){} }
