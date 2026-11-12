# Task 2 — Board Bring-Up and Hardware Diagnostics

## Current scaffold status

The supplied RT-Spark Spark-1 V1.0 schematic (HW1.1) was reviewed. It identifies an onboard AHT21 (compatible command/data protocol with the lab-requested AHT10), AP3216C, ICM-20608-G, buttons, discrete RGB LEDs, LED matrix, buzzer, ST7789V3 LCD, W25Q128 flash, microSD, RW007 Wi-Fi, and onboard ST-LINK. Firmware modules now configure the CPU clock, USART1, I2C2 and selected GPIO functions, sample sensor registers, and offer diagnostic/logger serial modes. Target build and live electrical behavior remain unverified.

## Verified pin summary

| Function | MCU mapping |
|---|---|
| AHT21 software I2C | PE0=SCL, PE1=SDA |
| AP3216C and ICM20608 I2C2 | PB10=SCL, PB11=SDA; addresses 0x1E and 0x68 |
| AP3216C interrupt / ICM interrupt | PG13 / PG12 |
| Buttons Up, Down, Left, Right | PC5, PC1, PC0, PC4; active-low per schematic pull arrangement |
| LED red, green, blue | PF12, PF14, PF11 |
| Buzzer | PB0 |
| Matrix enable/data | PF2 / PA7 |
| LCD reset, TE, backlight | PD3, PD6, PF9; LCD bus uses FSMC, NE3=PG10 and A18=PD13 |
| UART console | USART1 PA9 TX / PA10 RX, 115200 baud |
| SPI2 | PB13 SCK, PB14 MISO, PB15 MOSI, PB12 flash CS; PF10 Wi-Fi CS |
| HSE | 8 MHz; supplied schematic has onboard ST-LINK/V2.1 |

The LCD controller is ST7789V3 on FSMC with a 240×240 panel. A full LCD graphics/FSMC driver is not yet included; the current code only resets/backlights it. AHT sensor is marked AHT21 on this schematic, rather than AHT10.

### Required board details before driver implementation

The following items still require firmware implementation or hardware-side confirmation:

| Device | Required details |
|---|---|
| RTC | No discrete RTC identified; MCU RTC backup clock may be configured separately |
| External flash and microSD | W25Q128 over SPI2; microSD over SDIO; driver integration pending |
| Wi-Fi | RW007 over SPI2, PF10 CS, PG11 INT, PG15 reset; module protocol driver pending |
| LCD | ST7789V3/FSMC bus and reset/TE/backlight verified; rendering driver pending |
| GPIO polarity/functional check | Verify LED/matrix and buzzer active levels safely on target |

## Build and capture procedure

1. Install PlatformIO Core/VS Code extension and the `ststm32` platform, plus STM32Cube framework packages.
2. Fetch the pinned official FreeRTOS kernel, including ARM_CM4F and heap_4 sources, by running `powershell -ExecutionPolicy Bypass -File scripts/fetch_freertos.ps1` from the repository root. This requires network access to GitHub.
3. From the repository root run `pio run -e rt_spark_f407`; preserve the complete output as a build log.
4. Connect the board and use `pio device monitor -b 115200`; board console is USART1 on PA9/PA10 via the onboard ST-LINK virtual COM path. Store genuine output only; do not create synthetic readings.
5. Record board revision, power source, probe/firmware versions, date/time, sensor units and sample intervals, calibration offsets, and initialization/error states with the capture.

## Integration work still required

The next implementation stage must pin and integrate FreeRTOS kernel plus the ARM_CM4F portable layer, resolve the single-owner HAL/RTOS tick configuration, set NVIC priorities, and provide startup/linker files without duplicate exception handlers. After pinout confirmation, implement HAL drivers and interactive diagnostics for every fitted sensor, control, display, storage, and communication module. Do not treat absent or inoperative modules as available; document an instructor-verified exception instead.

## Live measurements

- Hardware bring-up status: `[HARDWARE_MEASUREMENT_REQUIRED]`
- Sensor readings and initialization status: `[HARDWARE_MEASUREMENT_REQUIRED]`
- UI self-test results: `[HARDWARE_MEASUREMENT_REQUIRED]`
