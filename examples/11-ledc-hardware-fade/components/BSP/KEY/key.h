#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
esp_err_t key_init(void);
// 由唯一任务每 10 ms 左右调用。返回一次稳定按下；内部不延时。
bool key_poll(int64_t now_us);
