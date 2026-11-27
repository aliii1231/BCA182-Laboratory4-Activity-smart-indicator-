#include "app.h"
#include "board_pins.h"
#include "board_out.h"
#include "model.h"
#include "product.h"
#include "led_matrix.h"
#include "lcd_st7789.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semphr.h"
#include "stream_buffer.h"

#ifndef GIT_COMMIT
#define GIT_COMMIT "unknown"
#endif

extern I2C_HandleTypeDef hi2c2;
extern UART_HandleTypeDef huart1;
void SystemClock_Config(void);
static RTC_HandleTypeDef hrtc;
static uint8_t rtc_ready=0;
static uint8_t diag_flash_ok=0, diag_sd_present=0, diag_sd_ok=0;
static StreamBufferHandle_t uart_rx_stream;
static uint8_t uart_rx_byte;
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
    uint8_t capacity_mb=0;
    if(HAL_SPI_Init(&hspi2_diag)==HAL_OK){HAL_GPIO_WritePin(GPIOB,GPIO_PIN_12,GPIO_PIN_RESET);HAL_StatusTypeDef st=HAL_SPI_TransmitReceive(&hspi2_diag,tx,rx,4,100);HAL_GPIO_WritePin(GPIOB,GPIO_PIN_12,GPIO_PIN_SET);if(st==HAL_OK){id[0]=rx[1];id[1]=rx[2];id[2]=rx[3];if(id[0]==0xEF&&id[1]==0x40){if(id[2]==0x16){capacity_mb=4;diag_flash_ok=1;}else if(id[2]==0x17){capacity_mb=8;diag_flash_ok=1;}else if(id[2]==0x18){capacity_mb=16;diag_flash_ok=1;}}}}
    App_Printf("FLASH,id=%02X%02X%02X,capacity_mb=%u,ok=%u\r\n",id[0],id[1],id[2],capacity_mb,diag_flash_ok);

    __HAL_RCC_SDIO_CLK_ENABLE();
    g.Pin=GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12;g.Mode=GPIO_MODE_AF_PP;g.Pull=GPIO_PULLUP;g.Speed=GPIO_SPEED_FREQ_VERY_HIGH;g.Alternate=GPIO_AF12_SDIO;HAL_GPIO_Init(GPIOC,&g);
    g.Pin=GPIO_PIN_2;HAL_GPIO_Init(GPIOD,&g);
    g.Pin=GPIO_PIN_3;g.Mode=GPIO_MODE_INPUT;g.Pull=GPIO_PULLUP;HAL_GPIO_Init(GPIOF,&g);
    diag_sd_present=(HAL_GPIO_ReadPin(GPIOF,GPIO_PIN_3)==GPIO_PIN_RESET);uint32_t blocks=0;uint8_t block[512];
    if(diag_sd_present){
      hsd_diag.Instance=SDIO;hsd_diag.Init.ClockEdge=SDIO_CLOCK_EDGE_RISING;hsd_diag.Init.ClockBypass=SDIO_CLOCK_BYPASS_DISABLE;
      hsd_diag.Init.ClockPowerSave=SDIO_CLOCK_POWER_SAVE_DISABLE;hsd_diag.Init.BusWide=SDIO_BUS_WIDE_1B;hsd_diag.Init.HardwareFlowControl=SDIO_HARDWARE_FLOW_CONTROL_DISABLE;hsd_diag.Init.ClockDiv=118;
      if(HAL_SD_Init(&hsd_diag)==HAL_OK && HAL_SD_ConfigWideBusOperation(&hsd_diag,SDIO_BUS_WIDE_4B)==HAL_OK){HAL_SD_CardInfoTypeDef info;if(HAL_SD_GetCardInfo(&hsd_diag,&info)==HAL_OK){blocks=(uint32_t)info.BlockNbr;diag_sd_ok=(HAL_SD_ReadBlocks(&hsd_diag,block,0,1,500)==HAL_OK);}}
    }
    App_Printf("SD,present=%u,blocks=%lu,ok=%u\r\n",diag_sd_present,(unsigned long)blocks,diag_sd_ok);
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

volatile uint8_t data_logger_mode=0;
static uint32_t data_sequence=0;
static SemaphoreHandle_t uart_mutex;
static volatile uint8_t alarm_muted=0;
static TaskHandle_t sensor_task_handle, infer_task_handle, ui_task_handle, console_task_handle;
char log_session[24]="unset";
volatile uint8_t log_label=255;
static void SensorTask(void *arg);
static void InferTask(void *arg);
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

static void App_PrintRaw(const char *data, size_t length)
{
    if (data == NULL || length == 0U) return;
    BaseType_t lock=(xTaskGetSchedulerState()!=taskSCHEDULER_NOT_STARTED && uart_mutex!=NULL);
    if(lock)xSemaphoreTake(uart_mutex,portMAX_DELAY);
    (void)HAL_UART_Transmit(&huart1,(uint8_t *)data,(uint16_t)length,1000U);
    if(lock)xSemaphoreGive(uart_mutex);
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
    (void)arg; TickType_t last=xTaskGetTickCount();
    TickType_t last_rtc=last;
    char ts[16], hs[16];
    uint8_t imu_ok=0;
    for (unsigned attempt=0; attempt<3 && !imu_ok; ++attempt) {
        if (ICM20608_Init()==HAL_OK) imu_ok=1;
        else if (attempt<2) vTaskDelay(pdMS_TO_TICKS(100));
    }
    App_Printf("BOOT,RT-SPARK,STM32F407ZGT6,clock_hz=%lu\r\n",(unsigned long)HAL_RCC_GetHCLKFreq());
    App_Printf("BUILD,commit=%s,date=%s %s\r\n",GIT_COMMIT,__DATE__,__TIME__);
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
        s.task_tick=xTaskGetTickCount();
        HAL_StatusTypeDef icm=ICM20608_ReadWhoAmI(&w);
        imu_raw_t imu={0};
        HAL_StatusTypeDef imu_status=(imu_ok && ICM20608_ReadRaw(&imu)==HAL_OK)?HAL_OK:HAL_ERROR;
        if(imu_status==HAL_OK) App_Printf("IMU,tick_ms=%lu,ax_mg=%ld,ay_mg=%ld,az_mg=%ld,gx_mdps=%ld,gy_mdps=%ld,gz_mdps=%ld,imu_temp_cx100=%ld,imu_ok=1\r\n",(unsigned long)HAL_GetTick(),(long)imu.ax*1000L/16384L,(long)imu.ay*1000L/16384L,(long)imu.az*1000L/16384L,(long)imu.gx*1000L/131L,(long)imu.gy*1000L/131L,(long)imu.gz*1000L/131L,(long)imu.temp*1000L/3268L+2500L);
        else App_Printf("IMU,tick_ms=%lu,imu_ok=0\r\n",(unsigned long)HAL_GetTick());
        Product_UpdateDiagnostics(imu_status==HAL_OK?&imu:NULL,
                                  (uint8_t)(imu_status==HAL_OK), rtc_ready,
                                  diag_flash_ok, diag_sd_present, diag_sd_ok);
        (void)Product_QueueSample(&s);
        if(data_logger_mode){
          if(aht_status==HAL_OK) App_Printf("DATA,%s,%u,%lu,%lu,%s,%s,%u,%u,%u,%u\r\n",log_session,(unsigned)log_label,(unsigned long)data_sequence++,(unsigned long)s.tick_ms,ts,hs,s.ambient_raw,s.proximity_raw,s.valid_mask,s.button_mask);
          else App_Printf("DATA,%s,%u,%lu,%lu,NA,NA,%u,%u,%u,%u\r\n",log_session,(unsigned)log_label,(unsigned long)data_sequence++,(unsigned long)s.tick_ms,s.ambient_raw,s.proximity_raw,s.valid_mask,s.button_mask);
        } else {
          if(aht_status==HAL_OK){fx(ts,sizeof ts,t,2);fx(hs,sizeof hs,h,2);App_Printf("SENSOR,tick_ms=%lu,temp_c=%s,rh_pct=%s,ambient_raw=%u,proximity_raw=%u,valid=0x%02X,icm_whoami=0x%02X,icm_ok=%u,interval_ms=1000,cal_temp_c=0,cal_rh_pct=0\r\n",(unsigned long)s.tick_ms,ts,hs,s.ambient_raw,s.proximity_raw,s.valid_mask,w,(icm==HAL_OK));}
          else App_Printf("SENSOR,tick_ms=%lu,temp_c=NA,rh_pct=NA,ambient_raw=%u,proximity_raw=%u,valid=0x%02X,aht_error=%u,icm_whoami=0x%02X,icm_ok=%u,interval_ms=1000,cal_temp_c=0,cal_rh_pct=0\r\n",(unsigned long)s.tick_ms,s.ambient_raw,s.proximity_raw,s.valid_mask,(unsigned)aht_status,w,(icm==HAL_OK));
        }
        vTaskDelayUntil(&last,pdMS_TO_TICKS(1000));
    }
}

static void InferTask(void *arg)
{
    Product_InferTask(arg);
}

static uint8_t product_view_changed(const product_status_t *a,const product_status_t *b,uint8_t page)
{
    const sensor_sample_t *x=&a->latest_sample,*y=&b->latest_sample;
    if(a->window_fill!=b->window_fill || a->sensor_fault!=b->sensor_fault ||
       x->valid_mask!=y->valid_mask || x->ambient_raw!=y->ambient_raw || x->proximity_raw!=y->proximity_raw ||
       (int32_t)(x->temperature_c*100.0f)!=(int32_t)(y->temperature_c*100.0f) ||
       (int32_t)(x->humidity_pct*100.0f)!=(int32_t)(y->humidity_pct*100.0f)) return 1;
    if(page==0) return (uint8_t)(a->shown_class!=b->shown_class);
    return (uint8_t)(a->imu_ok!=b->imu_ok || a->rtc_ok!=b->rtc_ok ||
                     a->flash_ok!=b->flash_ok || a->sd_present!=b->sd_present || a->sd_ok!=b->sd_ok);
}

static void UITask(void *arg)
{
    (void)arg; uint8_t page=0, acknowledged=0;
    uint8_t candidate=0, candidate_count=0, stable=0, screen_valid=0, blink_on=0, beep_active=0;
    uint8_t latency_state=PRODUCT_UNKNOWN_CLASS, latency_state_initialized=0;
    uint32_t last_draw=0, last_blink=0, last_beep=0, beep_start=0;
    product_status_t previous={0}, current={0};
    for(;;){
        uint8_t b=buttons();
        if(b==candidate){if(candidate_count<2U)++candidate_count;}
        else{candidate=b;candidate_count=1;}
        if(candidate_count>=2U && stable!=candidate){
            uint8_t pressed=(uint8_t)(candidate&(uint8_t)~stable);stable=candidate;
            if(pressed)App_Printf("BUTTON,tick_ms=%lu,mask=%u\r\n",(unsigned long)HAL_GetTick(),(unsigned)pressed);
            if(!data_logger_mode && (pressed&3U)){
                page=(uint8_t)!page;screen_valid=0;
                App_Printf("UI,tick_ms=%lu,page=%u\r\n",(unsigned long)HAL_GetTick(),page);
            }
            if(!data_logger_mode && (pressed&4U)){
                acknowledged=1;buzzer_tone(0);
                App_Printf("UI,tick_ms=%lu,ack=1\r\n",(unsigned long)HAL_GetTick());
            }
            if(!data_logger_mode && (pressed&8U) && page){Board_SelfTest();screen_valid=0;}
        }
        if(!data_logger_mode){
            Product_GetStatus(&current);
            uint32_t now=HAL_GetTick();
            if(!screen_valid || product_view_changed(&current,&previous,page) || (uint32_t)(now-last_draw)>=2000U){
                uint8_t visible_state=current.sensor_fault?PRODUCT_SENSOR_ERROR_STATE:current.shown_class;
                if(!screen_valid)lcd_fill(0x0000);
                if(page==0)lcd_show_product(&current);else lcd_show_diagnostics(&current);
                previous=current;last_draw=now;screen_valid=1;
                uint32_t ui_tick=xTaskGetTickCount();
                if(!latency_state_initialized){latency_state=visible_state;latency_state_initialized=1;}
                else if(visible_state!=latency_state){
                    latency_state=visible_state;
                    Product_SetUiTick(current.sample_tick,ui_tick);
                    App_Printf("LAT,tick_ms=%lu,shown=%u,sample_to_pred_ms=%lu,sample_to_lcd_ms=%lu\r\n",
                      (unsigned long)current.sample_tick,(unsigned)visible_state,
                      (unsigned long)(current.pred_done_tick-current.sample_tick),
                      (unsigned long)(ui_tick-current.sample_tick));
                }
            }
            if(current.sensor_fault){
                if((uint32_t)(now-last_blink)>=500U){blink_on=(uint8_t)!blink_on;last_blink=now;}
                led_set(LED_COLOR_BLUE,blink_on);led_set(LED_COLOR_RED,blink_on);beep_active=0;buzzer_alarm_tone(0);
            }else{
                led_set(LED_COLOR_BLUE,current.shown_class==1U);
                led_set(LED_COLOR_RED,current.shown_class==2U);
                if(current.shown_class!=2U){acknowledged=0;beep_active=0;buzzer_alarm_tone(0);}
                else if(acknowledged || alarm_muted){beep_active=0;buzzer_alarm_tone(0);}
                else if(beep_active && (uint32_t)(now-beep_start)>=300U){beep_active=0;buzzer_alarm_tone(0);}
                else if(!beep_active && (uint32_t)(now-last_beep)>=5000U){
                    beep_start=now;last_beep=now;beep_active=1;buzzer_alarm_tone(1);
                }
            }
        }else{
            screen_valid=0;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

static void ConsoleTask(void *arg)
{
    (void)arg; uint8_t ch; char line[40]; unsigned used=0; TickType_t last_report=xTaskGetTickCount();
    static char task_table[512];
    App_Printf("COMMANDS: d=diagnostic, l=<session>,<label 0-3> logger, x=stop logger, m=model self-test, q=mute toggle\r\n");
    for(;;){
      if(xStreamBufferReceive(uart_rx_stream,&ch,1,pdMS_TO_TICKS(100))==1){
        if(ch=='\r'||ch=='\n'){
          line[used]=0;
          if(line[0]=='l'&&line[1]=='='){
            char *comma=strchr(line+2,',');
            if(comma){*comma=0; unsigned lab=(unsigned)atoi(comma+1); size_t n=strlen(line+2);
              if(n>0&&n<sizeof(log_session)&&lab<=3){taskENTER_CRITICAL();memcpy(log_session,line+2,n+1);data_sequence=0;taskEXIT_CRITICAL();log_label=(uint8_t)lab;data_logger_mode=1;led_set(LED_COLOR_RED,0);led_set(LED_COLOR_BLUE,0);buzzer_tone(0);matrix_clear();lcd_show_recording(log_session,log_label);App_Printf("MODE,logger,session=%s,label=%u\r\n",log_session,lab);App_Printf("OUTPUTS,tick_ms=%lu,lcd=dark_static,backlight=on,matrix=off\r\n",(unsigned long)HAL_GetTick());App_Printf("CSV,session_id,label,sequence,tick_ms,temp_c,rh_pct,ambient_raw,proximity_raw,valid_mask,buttons\r\n");}
              else App_Printf("ERROR,logger_command\r\n");
            } else App_Printf("ERROR,expected_l=session,label\r\n");
          } else if(line[0]=='t'&&line[1]=='='){if(rtc_ready&&rtc_set_from_text(line+2))App_Printf("RTC,set=ok\r\n");else App_Printf("RTC,set=error\r\n");}
          else if(line[0]=='q'&&line[1]=='\0'){alarm_muted=(uint8_t)!alarm_muted;App_Printf("UI,tick_ms=%lu,mute=%u\r\n",(unsigned long)HAL_GetTick(),(unsigned)alarm_muted);}
          else if(line[0]=='b'&&line[1]=='\0'){if(!data_logger_mode)buzzer_test_sequence();}
          else if(line[0]=='f'&&line[1]=='\0'){
            if(!data_logger_mode){Product_StartFaultInjection(15000U);App_Printf("FAULTINJECT,start_tick_ms=%lu\r\n",(unsigned long)HAL_GetTick());}
          }
          else if(line[0]=='t'&&line[1]=='\0'){
            App_Printf("TASKS,name state prio stack_hw number\r\n");
            vTaskListTasks(task_table,sizeof(task_table));
            App_PrintRaw(task_table,strlen(task_table));
            App_Printf("TASKS,end\r\n");
          }
          else if(line[0]=='d'){data_logger_mode=0;lcd_show_idle();App_Printf("MODE,diagnostic\r\n");}
          else if(line[0]=='x'){data_logger_mode=0;lcd_show_idle();App_Printf("MODE,stopped\r\n");}
          else if(line[0]=='m'&&line[1]=='\0'){model_selftest();}
          used=0;
        } else if(used<sizeof(line)-1) line[used++]=(char)ch;
      }
      if((TickType_t)(xTaskGetTickCount()-last_report)>=pdMS_TO_TICKS(10000)){
        product_status_t st; Product_GetStatus(&st);
        App_Printf("PRODUCT,tick_ms=%lu,shown=%u,fault=%u,drops=%lu,stack_hw_sensor=%u,stack_hw_infer=%u,stack_hw_ui=%u,stack_hw_console=%u,heap_free=%lu,heap_min=%lu\r\n",
          (unsigned long)HAL_GetTick(),(unsigned)st.shown_class,(unsigned)st.sensor_fault,
          (unsigned long)Product_GetDropCount(),
          (unsigned)uxTaskGetStackHighWaterMark(sensor_task_handle),(unsigned)uxTaskGetStackHighWaterMark(infer_task_handle),
          (unsigned)uxTaskGetStackHighWaterMark(ui_task_handle),(unsigned)uxTaskGetStackHighWaterMark(console_task_handle),
          (unsigned long)xPortGetFreeHeapSize(),(unsigned long)xPortGetMinimumEverFreeHeapSize());
        last_report=xTaskGetTickCount();
      }
    }
}

void USART1_IRQHandler(void){HAL_UART_IRQHandler(&huart1);}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance==USART1){BaseType_t wake=pdFALSE;if(uart_rx_stream)xStreamBufferSendFromISR(uart_rx_stream,&uart_rx_byte,1,&wake);HAL_UART_Receive_IT(&huart1,&uart_rx_byte,1);portYIELD_FROM_ISR(wake);}
}

int main(void)
{
    HAL_Init(); SystemClock_Config(); Board_Peripherals_Init(); Board_UI_Init(); Board_Out_Init(); matrix_init(); rtc_init();
    if(!lcd_init()) App_Printf("INIT,lcd=error\r\n");
    lcd_show_idle();
    App_Printf("INIT,uart=ok,i2c2=%s,aht_soft_i2c=ready,lcd=initialized\r\n",HAL_I2C_GetState(&hi2c2)==HAL_I2C_STATE_READY?"ready":"error");
    uart_mutex=xSemaphoreCreateMutex();
    uart_rx_stream=xStreamBufferCreate(128,1);
    Product_Init();
    HAL_NVIC_SetPriority(USART1_IRQn,configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY,0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
    if(uart_mutex==NULL || uart_rx_stream==NULL || HAL_UART_Receive_IT(&huart1,&uart_rx_byte,1)!=HAL_OK ||
       xTaskCreate(SensorTask,"SensorTask",512,NULL,3,&sensor_task_handle)!=pdPASS ||
       xTaskCreate(InferTask,"InferTask",512,NULL,3,&infer_task_handle)!=pdPASS ||
       xTaskCreate(UITask,"UITask",512,NULL,2,&ui_task_handle)!=pdPASS ||
       xTaskCreate(ConsoleTask,"ConsoleTask",512,NULL,1,&console_task_handle)!=pdPASS) Error_Handler();
    vTaskStartScheduler(); Error_Handler();
}

void Error_Handler(void){__disable_irq();for(;;){} }
