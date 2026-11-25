#include "model.h"
#include "model_selftest_vectors.h"
#include "app.h"
#include <math.h>
#include <stdint.h>

void model_features(const sensor_sample_t w[MODEL_WINDOW_SAMPLES], float f[MODEL_N_FEATURES])
{
    float amb_sum = 0.0f, prox_sum = 0.0f, temp_sum = 0.0f, rh_sum = 0.0f;
    float amb_max = (float)w[0].ambient_raw, prox_max = (float)w[0].proximity_raw;
    float prox_min = (float)w[0].proximity_raw;
    for (unsigned i = 0; i < MODEL_WINDOW_SAMPLES; ++i) {
        const float amb = (float)w[i].ambient_raw;
        const float prox = (float)w[i].proximity_raw;
        amb_sum += amb;
        prox_sum += prox;
        temp_sum += w[i].temperature_c;
        rh_sum += w[i].humidity_pct;
        if (amb > amb_max) amb_max = amb;
        if (prox > prox_max) prox_max = prox;
        if (prox < prox_min) prox_min = prox;
    }
    const float n = (float)MODEL_WINDOW_SAMPLES;
    const float amb_mean = amb_sum / n;
    const float prox_mean = prox_sum / n;
    const float temp_mean = temp_sum / n;
    const float rh_mean = rh_sum / n;
    float amb_var = 0.0f, prox_var = 0.0f;
    for (unsigned i = 0; i < MODEL_WINDOW_SAMPLES; ++i) {
        const float da = (float)w[i].ambient_raw - amb_mean;
        const float dp = (float)w[i].proximity_raw - prox_mean;
        amb_var += da * da;
        prox_var += dp * dp;
    }
    f[0] = amb_mean;
    f[1] = amb_max;
    f[2] = sqrtf(amb_var / n);
    f[3] = prox_mean;
    f[4] = prox_max;
    f[5] = prox_min;
    f[6] = sqrtf(prox_var / n);
    f[7] = temp_mean;
    f[8] = rh_mean;
    f[9] = w[MODEL_WINDOW_SAMPLES - 1].temperature_c - w[0].temperature_c;
    f[10] = w[MODEL_WINDOW_SAMPLES - 1].humidity_pct - w[0].humidity_pct;
}

int model_predict(const float f[MODEL_N_FEATURES], float scores[MODEL_N_CLASSES])
{
    for (unsigned c = 0; c < MODEL_N_CLASSES; ++c) {
        float score = model_intercept[c];
        for (unsigned i = 0; i < MODEL_N_FEATURES; ++i) {
            const float z = (f[i] - model_mean[i]) / model_scale[i];
            score += model_coef[c][i] * z;
        }
        scores[c] = score;
    }
    int best = 0;
    for (int c = 1; c < MODEL_N_CLASSES; ++c) {
        if (scores[c] > scores[best]) best = c;
    }
    return best;
}

void model_selftest(void)
{
    static uint32_t cycles[50];
    unsigned matches = 0;
    float scores[MODEL_N_CLASSES];
    for (unsigned i = 0; i < 36U; ++i) {
        if (model_predict(selftest_features[i], scores) == (int)selftest_expected[i]) ++matches;
    }
    App_Printf("MODELTEST,n=36,match=%u,mismatch=%u\r\n", matches, 36U - matches);

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    for (unsigned i = 0; i < 50U; ++i) {
        const uint32_t start = DWT->CYCCNT;
        (void)model_predict(selftest_features[i % 36U], scores);
        cycles[i] = DWT->CYCCNT - start;
    }
    for (unsigned i = 1; i < 50U; ++i) {
        const uint32_t value = cycles[i];
        unsigned j = i;
        while (j > 0U && cycles[j - 1U] > value) {
            cycles[j] = cycles[j - 1U];
            --j;
        }
        cycles[j] = value;
    }
    const uint64_t median_cycles = ((uint64_t)cycles[24] + cycles[25]) / 2U;
    const uint32_t median_milli_us = (uint32_t)(median_cycles * 1000U / 168U);
    const uint32_t p95_milli_us = (uint32_t)((uint64_t)cycles[47] * 1000U / 168U);
    App_Printf("MODELTIME,n=50,median_us=%lu.%03lu,p95_us=%lu.%03lu\r\n",
               (unsigned long)(median_milli_us / 1000U),
               (unsigned long)(median_milli_us % 1000U),
               (unsigned long)(p95_milli_us / 1000U),
               (unsigned long)(p95_milli_us % 1000U));
    App_Printf("MODELSIZE,flash_const_bytes=%lu\r\n",
               (unsigned long)(sizeof(model_mean) + sizeof(model_scale) +
                               sizeof(model_coef) + sizeof(model_intercept)));
}
