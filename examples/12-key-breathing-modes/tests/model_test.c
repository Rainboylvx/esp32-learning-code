#include <assert.h>
#include <stdio.h>
#include "app_model.h"

int main(void)
{
    app_model_t model = {.mode = APP_OFF, .entered_at_us = 100};
    assert(app_model_brightness(&model, 100) == 0);
    app_model_next(&model, 200);
    assert(model.mode == APP_ON);
    assert(app_model_brightness(&model, 300) == 1023);
    app_model_next(&model, 10000000);
    assert(model.mode == APP_BREATH);
    assert(app_model_brightness(&model, 10000000) == 0);
    assert(app_model_brightness(&model, 10500000) == 255);
    assert(app_model_brightness(&model, 11000000) == 1023);
    assert(app_model_brightness(&model, 11500000) == 255);
    assert(app_model_brightness(&model, 12000000) == 0);
    // 跳过许多刷新事件，仍根据时间回到正确相位，而非累计 tick 次数。
    assert(app_model_brightness(&model, 41000000) == 1023);
    for (int64_t t = 0; t <= 4000000; t += 1000) {
        uint32_t level = app_model_brightness(&model, model.entered_at_us + t);
        assert(level <= 1023);
    }
    // 呼吸过程中切到 OFF，不等待呼吸周期结束。
    app_model_next(&model, 41500000);
    assert(model.mode == APP_OFF);
    assert(app_model_brightness(&model, 41500000) == 0);
    puts("model tests: PASS (modes, endpoints, range, skipped refresh)");
    return 0;
}
