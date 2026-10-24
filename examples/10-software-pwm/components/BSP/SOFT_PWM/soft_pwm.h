#pragma once
#include "esp_err.h"
// 20 个时间槽，逻辑亮度 0..20；GPIO1 由此模块独占。
#define SOFT_PWM_SLOTS 20U
esp_err_t soft_pwm_init(void);
esp_err_t soft_pwm_set(unsigned slots_on);
