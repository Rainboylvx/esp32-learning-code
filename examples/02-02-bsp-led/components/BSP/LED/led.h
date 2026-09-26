#pragma once

#include <stdbool.h>
#include "esp_err.h"

/**
 * @brief 初始化 DNESP32S3 板载红色 LED。
 *
 * @return ESP_OK 表示成功，其他值表示 GPIO 配置失败。
 */
esp_err_t led_init(void);

/**
 * @brief 设置板载红色 LED 的状态。
 *
 * 调用者只表达“亮”或“灭”，无需知道 GPIO 编号和有效电平。
 *
 * @param on true 点亮，false 熄灭。
 * @return ESP_OK 表示成功，其他值表示设置失败。
 */
esp_err_t led_set(bool on);
