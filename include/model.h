#ifndef MODEL_H
#define MODEL_H

#include "app.h"
#include "model_params.h"

void model_features(const sensor_sample_t w[MODEL_WINDOW_SAMPLES], float f[MODEL_N_FEATURES]);
int model_predict(const float f[MODEL_N_FEATURES], float scores[MODEL_N_CLASSES]);
void model_selftest(void);

#endif /* MODEL_H */
