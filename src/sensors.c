#include "app.h"
#include "FreeRTOS.h"
#include "task.h"
#include "board_pins.h"

extern I2C_HandleTypeDef hi2c2;
extern void App_Printf(const char *format, ...);

#define AHT_ADDR (0x38u << 1)
#define AP_ADDR  (0x1Eu << 1)
#define ICM_ADDR (0x68u << 1)

static void soft_delay(void) { for (volatile unsigned i = 0; i < 80; ++i) __NOP(); }
static void scl(uint8_t high) { HAL_GPIO_WritePin(AHT_GPIO, AHT_SCL_PIN, high ? GPIO_PIN_SET : GPIO_PIN_RESET); soft_delay(); }
static void sda(uint8_t high) { HAL_GPIO_WritePin(AHT_GPIO, AHT_SDA_PIN, high ? GPIO_PIN_SET : GPIO_PIN_RESET); soft_delay(); }
static uint8_t read_sda(void) { GPIO_InitTypeDef g={0}; g.Pin=AHT_SDA_PIN; g.Mode=GPIO_MODE_INPUT; g.Pull=GPIO_PULLUP; HAL_GPIO_Init(AHT_GPIO,&g); soft_delay(); uint8_t b=HAL_GPIO_ReadPin(AHT_GPIO,AHT_SDA_PIN); g.Mode=GPIO_MODE_OUTPUT_OD; HAL_GPIO_Init(AHT_GPIO,&g); return b; }
static void start(void) { sda(1); scl(1); sda(0); scl(0); }
static void stop(void) { sda(0); scl(1); sda(1); }
static uint8_t write_byte(uint8_t v) { for(int i=0;i<8;i++){ sda((v&0x80)!=0); scl(1); scl(0); v<<=1; } sda(1); scl(1); uint8_t ack=(uint8_t)!read_sda(); scl(0); return ack; }
static uint8_t read_byte(uint8_t ack) { uint8_t v=0; sda(1); for(int i=0;i<8;i++){ scl(1); v=(uint8_t)((v<<1)|read_sda()); scl(0); } sda(!ack); scl(1); scl(0); sda(1); return v; }

HAL_StatusTypeDef AHT_Read(float *temperature_c, float *humidity_pct)
{
    uint8_t d[6]; start(); if(!write_byte((uint8_t)(AHT_ADDR|0))) { stop(); return HAL_ERROR; }
    if(!write_byte(0xAC)||!write_byte(0x33)||!write_byte(0x00)){ stop(); return HAL_ERROR; } stop();
    vTaskDelay(pdMS_TO_TICKS(85)); start(); if(!write_byte((uint8_t)(AHT_ADDR|1))){ stop(); return HAL_ERROR; }
    for(int i=0;i<6;i++) { d[i]=read_byte(i<5); }
    stop();
    if(d[0]&0x80u) return HAL_BUSY;
    uint32_t rh=((uint32_t)d[1]<<12)|((uint32_t)d[2]<<4)|(d[3]>>4);
    uint32_t tr=((uint32_t)(d[3]&0x0Fu)<<16)|((uint32_t)d[4]<<8)|d[5];
    *humidity_pct=(float)rh*100.0f/1048576.0f;
    *temperature_c=(float)tr*200.0f/1048576.0f-50.0f;
    return (*temperature_c>=-40 && *temperature_c<=85 && *humidity_pct>=0 && *humidity_pct<=100) ? HAL_OK : HAL_ERROR;
}

static HAL_StatusTypeDef ap_write(uint8_t reg,uint8_t val){uint8_t b[2]={reg,val};return HAL_I2C_Master_Transmit(&hi2c2,AP_ADDR,b,2,50);}
static HAL_StatusTypeDef ap_read(uint8_t reg,uint8_t *v){return HAL_I2C_Mem_Read(&hi2c2,AP_ADDR,reg,I2C_MEMADD_SIZE_8BIT,v,1,50);}
HAL_StatusTypeDef AP3216C_Read(uint16_t *ambient,uint16_t *proximity)
{
    uint8_t als0,als1,ps0,ps1;
    HAL_StatusTypeDef st=ap_write(0x00,0x04); if(st!=HAL_OK)return st;
    vTaskDelay(pdMS_TO_TICKS(10));
    st=ap_write(0x00,0x03); if(st!=HAL_OK)return st;
    vTaskDelay(pdMS_TO_TICKS(120));
    if(ap_read(0x0C,&als0)!=HAL_OK||ap_read(0x0D,&als1)!=HAL_OK||ap_read(0x0E,&ps0)!=HAL_OK||ap_read(0x0F,&ps1)!=HAL_OK)return HAL_ERROR;
    *ambient=(uint16_t)(((uint16_t)als1<<8)|als0); *proximity=(uint16_t)(((ps1&0x3Fu)<<4)|(ps0&0x0Fu)); return HAL_OK;
}

HAL_StatusTypeDef ICM20608_ReadWhoAmI(uint8_t *whoami)
{
    if(HAL_I2C_Mem_Read(&hi2c2,ICM_ADDR,0x75,I2C_MEMADD_SIZE_8BIT,whoami,1,50)!=HAL_OK)return HAL_ERROR;
    return (*whoami==0xAFu) ? HAL_OK : HAL_ERROR;
}
