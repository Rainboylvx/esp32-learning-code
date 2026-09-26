#pragma once

#include "esp_err.h"

/**
 * @brief 按键扫描产生的事件。
 */
typedef enum {
    KEY_EVENT_NONE = 0,       // 当前没有新的按下事件。
    KEY_EVENT_BOOT_PRESS,     // BOOT 完成了一次有效按下。
} key_event_t;

/**
 * @brief 初始化 BOOT 按键对应的 GPIO0 输入和内部上拉。
 */
esp_err_t key_init(void);

/**
 * @brief 轮询 BOOT 按键，并对按下和松开过程进行软件消抖。
 *
 * 一次按住只产生一个 KEY_EVENT_BOOT_PRESS；松开并再次按下后，
 * 才会产生下一个事件。
 */
key_event_t key_scan(void);
