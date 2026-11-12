#include "app.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

void vApplicationMallocFailedHook(void) { Error_Handler(); }
void vApplicationStackOverflowHook(TaskHandle_t task, char *name) { (void)task; (void)name; Error_Handler(); }
