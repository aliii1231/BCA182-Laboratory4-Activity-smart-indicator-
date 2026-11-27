#include "product.h"
#include "model.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include <string.h>

extern volatile uint8_t data_logger_mode;

static QueueHandle_t sampleQ;
static SemaphoreHandle_t status_mutex;
static SemaphoreHandle_t diagnostics_mutex;
static product_status_t status;
static product_status_t diagnostics;
static uint32_t dropped_samples;
static volatile uint8_t fault_injection_active;
static volatile TickType_t fault_injection_deadline;

void Product_Init(void)
{
    memset(&status, 0, sizeof(status));
    status.predicted_class = PRODUCT_UNKNOWN_CLASS;
    status.shown_class = PRODUCT_UNKNOWN_CLASS;
    status_mutex = xSemaphoreCreateMutex();
    diagnostics_mutex = xSemaphoreCreateMutex();
    sampleQ = xQueueCreate(4, sizeof(sensor_sample_t));
    if (status_mutex == NULL || diagnostics_mutex == NULL || sampleQ == NULL) Error_Handler();
}

BaseType_t Product_QueueSample(const sensor_sample_t *sample)
{
    if (sampleQ == NULL || sample == NULL) return pdFAIL;
    if (xQueueSend(sampleQ, sample, 0) == pdPASS) return pdPASS;
    taskENTER_CRITICAL();
    ++dropped_samples;
    taskEXIT_CRITICAL();
    return pdFAIL;
}

uint32_t Product_GetDropCount(void)
{
    uint32_t count;
    taskENTER_CRITICAL();
    count = dropped_samples;
    taskEXIT_CRITICAL();
    return count;
}

void Product_SetUiTick(uint32_t sample_tick, uint32_t ui_tick)
{
    if (status_mutex == NULL) return;
    xSemaphoreTake(status_mutex, portMAX_DELAY);
    if (status.sample_tick == sample_tick) status.ui_tick = ui_tick;
    xSemaphoreGive(status_mutex);
}

void Product_StartFaultInjection(uint32_t duration_ms)
{
    taskENTER_CRITICAL();
    fault_injection_deadline = xTaskGetTickCount() + pdMS_TO_TICKS(duration_ms);
    fault_injection_active = 1;
    taskEXIT_CRITICAL();
}

void Product_GetStatus(product_status_t *out)
{
    if (out == NULL) return;
    if (status_mutex == NULL) {
        *out = status;
        out->drop_count = Product_GetDropCount();
        return;
    }
    xSemaphoreTake(status_mutex, portMAX_DELAY);
    *out = status;
    xSemaphoreGive(status_mutex);
    xSemaphoreTake(diagnostics_mutex, portMAX_DELAY);
    out->imu = diagnostics.imu;
    out->imu_ok = diagnostics.imu_ok;
    out->rtc_ok = diagnostics.rtc_ok;
    out->flash_ok = diagnostics.flash_ok;
    out->sd_present = diagnostics.sd_present;
    out->sd_ok = diagnostics.sd_ok;
    xSemaphoreGive(diagnostics_mutex);
    out->drop_count = Product_GetDropCount();
}

void Product_UpdateDiagnostics(const imu_raw_t *imu, uint8_t imu_ok,
                              uint8_t rtc_ok, uint8_t flash_ok,
                              uint8_t sd_present, uint8_t sd_ok)
{
    if (diagnostics_mutex == NULL) return;
    xSemaphoreTake(diagnostics_mutex, portMAX_DELAY);
    if (imu != NULL) diagnostics.imu = *imu;
    diagnostics.imu_ok = imu_ok;
    diagnostics.rtc_ok = rtc_ok;
    diagnostics.flash_ok = flash_ok;
    diagnostics.sd_present = sd_present;
    diagnostics.sd_ok = sd_ok;
    xSemaphoreGive(diagnostics_mutex);
}

static uint8_t sample_valid(const sensor_sample_t *sample)
{
    return (uint8_t)(((sample->valid_mask & 0x03U) == 0x03U) &&
                     sample->temperature_c >= -40.0f && sample->temperature_c <= 85.0f &&
                     sample->humidity_pct >= 0.0f && sample->humidity_pct <= 100.0f);
}

static int32_t score_milli(float score)
{
    float scaled = score * 1000.0f;
    return (int32_t)(scaled >= 0.0f ? scaled + 0.5f : scaled - 0.5f);
}

void Product_InferTask(void *arg)
{
    (void)arg;
    sensor_sample_t window[MODEL_WINDOW_SAMPLES];
    uint8_t fill = 0, next = 0, consecutive = 0;
    uint8_t previous_prediction = PRODUCT_UNKNOWN_CLASS;
    uint32_t previous_tick = 0;
    uint8_t have_previous_tick = 0;
    uint8_t fault_latched = 0;
    uint32_t feat_us = 0;
    float features[MODEL_N_FEATURES], scores[MODEL_N_CLASSES];

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    for (;;) {
        sensor_sample_t sample;
        if (xQueueReceive(sampleQ, &sample, portMAX_DELAY) != pdPASS) continue;
        uint8_t predict = 0;
        uint8_t predicted = PRODUCT_UNKNOWN_CLASS;
        TickType_t infer_tick = xTaskGetTickCount();
        uint32_t sample_age_ms=(uint32_t)(HAL_GetTick()-sample.tick_ms);
        uint32_t sample_rtos_tick=(uint32_t)(infer_tick-sample_age_ms);
        uint8_t injected = 0;
        if (fault_injection_active) {
            if ((int32_t)(infer_tick - fault_injection_deadline) >= 0) {
                taskENTER_CRITICAL(); fault_injection_active = 0; taskEXIT_CRITICAL();
                if (!data_logger_mode)
                    App_Printf("FAULTINJECT,end_tick_ms=%lu\r\n", (unsigned long)HAL_GetTick());
            } else injected = 1;
        }
        if (data_logger_mode) {
            fill = next = consecutive = 0;
            previous_prediction = PRODUCT_UNKNOWN_CLASS;
            have_previous_tick = 0;
            fault_latched = 1;
            xSemaphoreTake(status_mutex, portMAX_DELAY);
            status.tick_ms = sample.tick_ms;
            status.sample_tick = sample_rtos_tick;
            status.pred_done_tick = infer_tick;
            status.latest_sample = sample;
            status.predicted_class = PRODUCT_UNKNOWN_CLASS;
            status.sensor_fault = fault_latched;
            status.window_fill = 0;
            status.feat_us = feat_us;
            status.drop_count = Product_GetDropCount();
            xSemaphoreGive(status_mutex);
            continue;
        }

        uint8_t bad_gap = (uint8_t)(have_previous_tick &&
                            (uint32_t)(sample.tick_ms - previous_tick) > 2500U);
        previous_tick = sample.tick_ms;
        have_previous_tick = 1;
        if (!sample_valid(&sample) || bad_gap || injected) {
            fill = next = consecutive = 0;
            previous_prediction = PRODUCT_UNKNOWN_CLASS;
            fault_latched = 1;
            xSemaphoreTake(status_mutex, portMAX_DELAY);
            status.tick_ms = sample.tick_ms;
            status.sample_tick = sample_rtos_tick;
            status.pred_done_tick = xTaskGetTickCount();
            status.predicted_class = PRODUCT_UNKNOWN_CLASS;
            status.sensor_fault = 1;
            status.window_fill = 0;
            status.latest_sample = sample;
            status.feat_us = feat_us;
            status.drop_count = Product_GetDropCount();
            xSemaphoreGive(status_mutex);
        } else {
            window[next] = sample;
            next = (uint8_t)((next + 1U) % MODEL_WINDOW_SAMPLES);
            if (fill < MODEL_WINDOW_SAMPLES) ++fill;
            if (fill == MODEL_WINDOW_SAMPLES) fault_latched = 0;
            if (fill == MODEL_WINDOW_SAMPLES) {
                sensor_sample_t ordered[MODEL_WINDOW_SAMPLES];
                for (unsigned i = 0; i < MODEL_WINDOW_SAMPLES; ++i)
                    ordered[i] = window[(next + i) % MODEL_WINDOW_SAMPLES];
                uint32_t start = DWT->CYCCNT;
                model_features(ordered, features);
                uint32_t cycles = DWT->CYCCNT - start;
                uint32_t hz = HAL_RCC_GetHCLKFreq();
                feat_us = hz ? (uint32_t)(((uint64_t)cycles * 1000000ULL) / hz) : 0U;
                int result = model_predict(features, scores);
                if (result >= 0 && result < MODEL_N_CLASSES) {
                    predicted = (uint8_t)result;
                    predict = 1;
                    if (predicted == previous_prediction) {
                        if (consecutive < 2U) ++consecutive;
                    } else {
                        previous_prediction = predicted;
                        consecutive = 1;
                    }
                }
            }

            xSemaphoreTake(status_mutex, portMAX_DELAY);
            status.tick_ms = sample.tick_ms;
            status.sample_tick = sample_rtos_tick;
            status.pred_done_tick = xTaskGetTickCount();
            status.latest_sample = sample;
            status.window_fill = fill;
            status.sensor_fault = fault_latched;
            status.feat_us = feat_us;
            if (predict) {
                status.predicted_class = predicted;
                for (unsigned i = 0; i < MODEL_N_CLASSES; ++i)
                    status.scores_milli[i] = score_milli(scores[i]);
                if (consecutive >= 2U) status.shown_class = predicted;
            } else if (fill < MODEL_WINDOW_SAMPLES) {
                status.predicted_class = PRODUCT_UNKNOWN_CLASS;
            }
            status.drop_count = Product_GetDropCount();
            xSemaphoreGive(status_mutex);
        }

        product_status_t snapshot;
        Product_GetStatus(&snapshot);
        if (!data_logger_mode) {
            App_Printf("PRED,tick_ms=%lu,fill=%u,pred=%u,shown=%u,s0=%ld,s1=%ld,s2=%ld,feat_us=%lu,fault=%u\r\n",
                       (unsigned long)sample.tick_ms, (unsigned)fill, (unsigned)snapshot.predicted_class,
                       (unsigned)snapshot.shown_class, (long)snapshot.scores_milli[0],
                       (long)snapshot.scores_milli[1], (long)snapshot.scores_milli[2],
                       (unsigned long)feat_us, (unsigned)snapshot.sensor_fault);
        }
    }
}
