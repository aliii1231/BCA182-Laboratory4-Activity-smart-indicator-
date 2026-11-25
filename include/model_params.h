/*
 * Model family: logistic
 * C: 0.03
 * sklearn version: 1.9.1
 * Git commit: 8621109ad4bb7e4df47223ff6e14c9e56e12b4a5
 * Timestamp UTC: 2026-11-25T12:22:55.822394+00:00
 * GENERATED - do not edit
 * Feature order: amb_mean, amb_max, amb_std, prox_mean, prox_max, prox_min, prox_std, temp_mean, rh_mean, temp_delta, rh_delta
 * Window statistics use population std (ddof=0); delta = last minus first.
 */
#ifndef MODEL_PARAMS_H
#define MODEL_PARAMS_H

#define MODEL_N_FEATURES 11
#define MODEL_N_CLASSES 3
#define MODEL_WINDOW_SAMPLES 10

static const float model_mean[11] = {23.5794118f, 29.5196078f, 3.69370669f, 78.9098039f, 243.441176f, 9.14705882f, 90.4146622f, 32.1789833f, 83.7696147f, 0.0155196078f, 0.116941176f};
static const float model_scale[11] = {89.373822f, 97.6605496f, 7.60666384f, 131.453798f, 396.255626f, 7.84581449f, 161.323775f, 0.837187003f, 5.21644349f, 0.0506060778f, 0.620571461f};
static const float model_coef[3][11] = {
    {-0.0821737574f, -0.0948164258f, -0.175341266f, -0.111407234f, -0.177225473f, 0.181651714f, -0.160166407f, -0.175418233f, -0.31319329f, -0.0567988513f, -0.0364899153f},
    {0.112286645f, 0.131275179f, 0.253773619f, 0.16313419f, 0.254738809f, -0.33419607f, 0.236108007f, -0.118298385f, -0.14350502f, -0.123198902f, 0.0147336318f},
    {-0.030112888f, -0.0364587534f, -0.0784323533f, -0.0517269555f, -0.0775133355f, 0.152544355f, -0.0759415998f, 0.293716619f, 0.45669831f, 0.179997754f, 0.0217562834f},
};
static const float model_intercept[3] = {0.190704031f, -0.0507656898f, -0.139938342f};

#endif /* MODEL_PARAMS_H */
