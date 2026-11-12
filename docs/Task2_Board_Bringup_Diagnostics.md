# Task 2 — Board Bring-Up and Hardware Diagnostics

## Current scaffold status

The repository has no RT-Spark schematic, verified pin table, board manifest, or hardware inventory. The custom PlatformIO board target reserves the STM32F407ZGT6 memory sizes and ST-LINK protocol, but its clock source, actual ST-LINK wiring/configuration, pinout, and peripheral wiring remain unverified. Firmware is deliberately a startup skeleton and does not initialize peripherals or claim live readings.

### Required board details before driver implementation

Please provide the RT-Spark (Spark-1) schematic or confirm the exact mappings for:

| Device | Required details |
|---|---|
| AHT10, AP3216C, ICM20608 | I2C/SPI instance, SCL/SDA or SCK/MISO/MOSI/CS pins, address/CS, interrupt pins, bus voltage |
| LCD | Controller/interface, bus pins, reset/backlight pins |
| Buttons | GPIO pins, active level, pull configuration |
| LEDs / matrix | GPIO/PWM/driver interface and pins |
| Buzzer | GPIO/PWM pin and active level |
| RTC, external flash, microSD/TF | Device/interface, chip selects, pins, fitted status |
| Wi-Fi, UART/USB | Module/interface, pins, console baud and connector mapping |
| Clock and debug | HSE frequency/source, ST-LINK connector/pin mapping |

## Build and capture procedure

1. Install PlatformIO Core/VS Code extension and the `ststm32` platform, plus STM32Cube framework packages.
2. From the repository root run `pio run -e rt_spark_f407`; preserve the complete output as a build log. A successful hardware build cannot be asserted until the custom manifest and framework source integration are validated against the installed PlatformIO version.
3. Once UART mapping and baud are confirmed and diagnostic firmware is implemented, connect the board and capture `pio device monitor -b <confirmed-baud> | Tee-Object docs/bringup-live.log` in PowerShell. Store genuine output only; do not create synthetic readings.
4. Record board revision, power source, probe/firmware versions, date/time, sensor units and sample intervals, calibration offsets, and initialization/error states with the capture.

## Integration work still required

The next implementation stage must pin and integrate FreeRTOS kernel plus the ARM_CM4F portable layer, resolve the single-owner HAL/RTOS tick configuration, set NVIC priorities, and provide startup/linker files without duplicate exception handlers. After pinout confirmation, implement HAL drivers and interactive diagnostics for every fitted sensor, control, display, storage, and communication module. Do not treat absent or inoperative modules as available; document an instructor-verified exception instead.

## Live measurements

- Hardware bring-up status: `[HARDWARE_MEASUREMENT_REQUIRED]`
- Sensor readings and initialization status: `[HARDWARE_MEASUREMENT_REQUIRED]`
- UI self-test results: `[HARDWARE_MEASUREMENT_REQUIRED]`
