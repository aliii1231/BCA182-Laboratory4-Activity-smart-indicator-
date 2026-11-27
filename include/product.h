#ifndef PRODUCT_H
#define PRODUCT_H

#include "app.h"

#define PRODUCT_UNKNOWN_CLASS 255U
#define PRODUCT_SENSOR_ERROR_STATE 254U

typedef struct product_status_t {
    uint32_t tick_ms;
    uint8_t predicted_class;
    uint8_t shown_class;
    int32_t scores_milli[3];
    uint8_t sensor_fault;
    uint8_t window_fill;
    uint32_t drop_count;
    uint32_t feat_us;
    uint32_t sample_tick;
    uint32_t pred_done_tick;
    uint32_t ui_tick;
    sensor_sample_t latest_sample;
    imu_raw_t imu;
    uint8_t imu_ok;
    uint8_t rtc_ok;
    uint8_t flash_ok;
    uint8_t sd_present;
    uint8_t sd_ok;
} product_status_t;

void Product_Init(void);
BaseType_t Product_QueueSample(const sensor_sample_t *sample);
void Product_InferTask(void *arg);
void Product_GetStatus(product_status_t *status);
void Product_UpdateDiagnostics(const imu_raw_t *imu, uint8_t imu_ok,
                              uint8_t rtc_ok, uint8_t flash_ok,
                              uint8_t sd_present, uint8_t sd_ok);
uint32_t Product_GetDropCount(void);
void Product_SetUiTick(uint32_t sample_tick, uint32_t ui_tick);
void Product_StartFaultInjection(uint32_t duration_ms);

#endif
