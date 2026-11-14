#include "app.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_sram.h"
#include "FreeRTOS.h"
#include "task.h"
#include "board_pins.h"
#include "lcd_st7789.h"
#include <stdio.h>

#define LCD_CMD (*((volatile uint8_t *)0x68000000U))
#define LCD_DATA (*((volatile uint8_t *)0x68040000U))
#define LCD_W 240U
#define LCD_H 240U
static uint16_t row_offset=0;

static void cmd(uint8_t c){LCD_CMD=c;}
static void data(uint8_t d){LCD_DATA=d;}
static void write_reg(uint8_t c,uint8_t d){cmd(c);data(d);}
static void address_window(uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1)
{
    cmd(0x2A);data(x0>>8);data(x0);data(x1>>8);data(x1);
    y0=(uint16_t)(y0+row_offset); y1=(uint16_t)(y1+row_offset);
    cmd(0x2B);data(y0>>8);data(y0);data(y1>>8);data(y1);cmd(0x2C);
}

static void fsmc_init(void)
{
    __HAL_RCC_FSMC_CLK_ENABLE(); __HAL_RCC_GPIOD_CLK_ENABLE(); __HAL_RCC_GPIOE_CLK_ENABLE(); __HAL_RCC_GPIOG_CLK_ENABLE();
    GPIO_InitTypeDef g={0}; g.Mode=GPIO_MODE_AF_PP;g.Pull=GPIO_NOPULL;g.Speed=GPIO_SPEED_FREQ_VERY_HIGH;g.Alternate=GPIO_AF12_FSMC;
    g.Pin=GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;HAL_GPIO_Init(GPIOD,&g);
    g.Pin=GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10;HAL_GPIO_Init(GPIOE,&g);
    g.Pin=GPIO_PIN_10;HAL_GPIO_Init(GPIOG,&g);
    FSMC_NORSRAM_TimingTypeDef t={0}; FSMC_NORSRAM_InitTypeDef n={0};
    n.NSBank=FSMC_NORSRAM_BANK3;n.DataAddressMux=FSMC_DATA_ADDRESS_MUX_DISABLE;n.MemoryType=FSMC_MEMORY_TYPE_SRAM;
    n.MemoryDataWidth=FSMC_NORSRAM_MEM_BUS_WIDTH_8;n.BurstAccessMode=FSMC_BURST_ACCESS_MODE_DISABLE;
    n.WaitSignalPolarity=FSMC_WAIT_SIGNAL_POLARITY_LOW;n.WrapMode=FSMC_WRAP_MODE_DISABLE;n.WaitSignalActive=FSMC_WAIT_TIMING_BEFORE_WS;
    n.WriteOperation=FSMC_WRITE_OPERATION_ENABLE;n.WaitSignal=FSMC_WAIT_SIGNAL_DISABLE;n.ExtendedMode=FSMC_EXTENDED_MODE_DISABLE;
    n.AsynchronousWait=FSMC_ASYNCHRONOUS_WAIT_DISABLE;n.WriteBurst=FSMC_WRITE_BURST_DISABLE;
    t.AddressSetupTime=5;t.AddressHoldTime=1;t.DataSetupTime=7;t.BusTurnAroundDuration=1;t.CLKDivision=2;t.DataLatency=2;t.AccessMode=FSMC_ACCESS_MODE_A;
    SRAM_HandleTypeDef h={0};h.Instance=FSMC_NORSRAM_DEVICE;h.Extended=FSMC_NORSRAM_EXTENDED_DEVICE;h.Init=n;
    if(HAL_SRAM_Init(&h,&t,NULL)!=HAL_OK)Error_Handler();
}

int lcd_init(void)
{
    HAL_GPIO_WritePin(LCD_BACKLIGHT_PORT,LCD_BACKLIGHT_PIN,GPIO_PIN_RESET);
    fsmc_init();
    HAL_GPIO_WritePin(LCD_RESET_PORT,LCD_RESET_PIN,GPIO_PIN_RESET);HAL_Delay(10);HAL_GPIO_WritePin(LCD_RESET_PORT,LCD_RESET_PIN,GPIO_PIN_SET);HAL_Delay(120);
    cmd(0x11);HAL_Delay(120);write_reg(0x3A,0x05);write_reg(0x36,0x00);cmd(0x21);cmd(0x29);HAL_Delay(20);
    row_offset=0;address_window(0,0,LCD_W-1,LCD_H-1);data(0); /* harmless address window setup */
    HAL_GPIO_WritePin(LCD_BACKLIGHT_PORT,LCD_BACKLIGHT_PIN,GPIO_PIN_SET);
    return 1;
}

void lcd_fill(uint16_t color)
{
    address_window(0,0,LCD_W-1,LCD_H-1);
    for(uint32_t i=0;i<LCD_W*LCD_H;i++){data(color>>8);data(color&0xFF);}
}

/* Minimal 5x7 glyphs in an 8x8 cell; unsupported characters render as '?'. */
static const uint8_t font[96][5]={
 [0]={0,0,0,0,0}, ['?'-32]={2,1,0x51,9,6}, ['0'-32]={0x3E,0x51,0x49,0x45,0x3E}, ['1'-32]={0,0x42,0x7F,0x40,0},
 ['2'-32]={0x42,0x61,0x51,0x49,0x46}, ['3'-32]={0x21,0x41,0x45,0x4B,0x31}, ['4'-32]={0x18,0x14,0x12,0x7F,0x10},
 ['5'-32]={0x27,0x45,0x45,0x45,0x39}, ['6'-32]={0x3C,0x4A,0x49,0x49,0x30}, ['7'-32]={1,0x71,9,5,3},
 ['8'-32]={0x36,0x49,0x49,0x49,0x36}, ['9'-32]={6,0x49,0x49,0x29,0x1E}, ['A'-32]={0x7E,0x11,0x11,0x11,0x7E},
 ['B'-32]={0x7F,0x49,0x49,0x49,0x36}, ['C'-32]={0x3E,0x41,0x41,0x41,0x22}, ['D'-32]={0x7F,0x41,0x41,0x22,0x1C},
 ['E'-32]={0x7F,0x49,0x49,0x49,0x41}, ['F'-32]={0x7F,9,9,9,1}, ['G'-32]={0x3E,0x41,0x49,0x49,0x7A},
 ['H'-32]={0x7F,8,8,8,0x7F}, ['I'-32]={0,0x41,0x7F,0x41,0}, ['L'-32]={0x7F,0x40,0x40,0x40,0x40},
 ['M'-32]={0x7F,2,0x0C,2,0x7F}, ['N'-32]={0x7F,4,8,0x10,0x7F}, ['O'-32]={0x3E,0x41,0x41,0x41,0x3E},
 ['P'-32]={0x7F,9,9,9,6}, ['R'-32]={0x7F,9,0x19,0x29,0x46}, ['S'-32]={0x46,0x49,0x49,0x49,0x31},
 ['T'-32]={1,1,0x7F,1,1}, ['U'-32]={0x3F,0x40,0x40,0x40,0x3F}, ['Y'-32]={3,4,0x78,4,3},
 ['a'-32]={0x20,0x54,0x54,0x54,0x78}, ['b'-32]={0x7F,0x48,0x44,0x44,0x38}, ['c'-32]={0x38,0x44,0x44,0x44,0x20},
 ['e'-32]={0x38,0x54,0x54,0x54,0x18}, ['h'-32]={0x7F,8,4,4,0x78}, ['i'-32]={0,0x44,0x7D,0x40,0},
 ['m'-32]={0x7C,4,0x18,4,0x78}, ['n'-32]={0x7C,8,4,4,0x78}, ['o'-32]={0x38,0x44,0x44,0x44,0x38},
 ['r'-32]={0x7C,8,4,4,8}, ['s'-32]={0x48,0x54,0x54,0x54,0x20}, ['t'-32]={4,0x3F,0x44,0x40,0x20}, ['u'-32]={0x3C,0x40,0x40,0x20,0x7C}
};

void lcd_draw_string(uint16_t x,uint16_t y,const char *str,uint16_t fg,uint16_t bg)
{
    while(*str&&x+8<=LCD_W){uint8_t c=(uint8_t)*str++;if(c<32||c>127)c='?';uint8_t idx=(uint8_t)(c-32);address_window(x,y,x+7,y+7);
      for(uint8_t row=0;row<8;row++) {
        for(uint8_t col=0;col<8;col++) { uint8_t on=(col<5&&row<7)?((font[idx][col]>>row)&1U):0;uint16_t px=on?fg:bg;data(px>>8);data(px&255); }
      }
      x+=8;
    }
}

void lcd_status(uint8_t state,int32_t temp_cx100,int32_t rh_cx100,uint16_t ambient)
{
    char line[32];snprintf(line,sizeof line,"State %u",(unsigned)state);lcd_draw_string(4,4,line,0xFFFF,0x0000);
    snprintf(line,sizeof line,"T %ld.%02ld C",(long)(temp_cx100/100),(long)(temp_cx100<0?-(temp_cx100%100):temp_cx100%100));lcd_draw_string(4,20,line,0x07FF,0x0000);
    snprintf(line,sizeof line,"RH %ld.%02ld",(long)(rh_cx100/100),(long)(rh_cx100<0?-(rh_cx100%100):rh_cx100%100));lcd_draw_string(4,36,line,0x07E0,0x0000);
    snprintf(line,sizeof line,"ALS %u",(unsigned)ambient);lcd_draw_string(4,52,line,0xFFE0,0x0000);
}

void lcd_test(void)
{
    lcd_fill(0xF800);vTaskDelay(pdMS_TO_TICKS(50));lcd_fill(0x07E0);vTaskDelay(pdMS_TO_TICKS(50));lcd_fill(0x001F);vTaskDelay(pdMS_TO_TICKS(50));lcd_fill(0);lcd_draw_string(4,4,"LCD TEST",0xFFFF,0);
}
