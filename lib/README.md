# Third-party libraries

Run `powershell -ExecutionPolicy Bypass -File scripts/fetch_freertos.ps1` from the repository root to download official FreeRTOS-Kernel `V11.1.0` into `lib/FreeRTOS/`. The archive supplies the kernel, `portable/GCC/ARM_CM4F/port.c`, and `portable/MemMang/heap_4.c`. The generated third-party tree is ignored by Git; this repository keeps the reproducible fetch procedure and configuration.

The PlatformIO environment uses `-I lib/FreeRTOS/include`, `-I lib/FreeRTOS/portable/GCC/ARM_CM4F`, `lib_extra_dirs = lib`, and `lib_ldf_mode = off`. After installation, run `pio run -e rt_spark_f407`.
