#pragma once
#include <stdint.h>
#include "esp_err.h"
// 10 位占空比分辨率，统一约定 0=灭、1023=最大亮度。
#define PWM_LED_MAX 1023U
esp_err_t pwm_led_init(void);
esp_err_t pwm_led_set(uint32_t brightness);
