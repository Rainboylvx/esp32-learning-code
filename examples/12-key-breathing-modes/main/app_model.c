#include "app_model.h"

void app_model_next(app_model_t *model, int64_t now_us)
{
    model->mode = (model->mode + 1) % 3;
    model->entered_at_us = now_us;
}

uint32_t app_model_brightness(const app_model_t *model, int64_t now_us)
{
    if (model->mode == APP_OFF) return 0;
    if (model->mode == APP_ON) return 1023;
    // 一个呼吸周期 2 秒。按绝对经过时间计算，避免漏事件导致相位漂移。
    int64_t phase = (now_us - model->entered_at_us) % 2000000;
    int64_t ramp = phase <= 1000000 ? phase : 2000000 - phase;
    // 平方曲线改善暗部层次；所有乘法在 int64_t 中进行。
    return (uint32_t)(ramp * ramp * 1023 / 1000000000000LL);
}
