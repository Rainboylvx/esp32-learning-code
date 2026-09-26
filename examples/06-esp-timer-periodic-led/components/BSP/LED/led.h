#pragma once

#include <stdbool.h>
#include "esp_err.h"

/**
 * @brief 初始化 DNESP32S3 板载红色 LED，初始化后默认熄灭。
 */
esp_err_t led_init(void);

/**
 * @brief 设置板载红色 LED 的逻辑状态。
 *
 * @param on true 点亮，false 熄灭。
 */
esp_err_t led_set(bool on);
