#pragma once
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// 创建后持续运行；receiver 必须在整个定时器生命周期内有效。
esp_err_t tick_start(TaskHandle_t receiver);
