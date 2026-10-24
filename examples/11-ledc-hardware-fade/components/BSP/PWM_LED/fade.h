#pragma once
#include "pwm_led.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
esp_err_t fade_init(TaskHandle_t receiver);
esp_err_t fade_to(uint32_t brightness, int duration_ms);
esp_err_t fade_stop(void);
