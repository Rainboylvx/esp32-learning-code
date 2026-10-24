#pragma once
#include <stdint.h>
typedef enum { APP_OFF, APP_ON, APP_BREATH } app_mode_t;
typedef struct {
    app_mode_t mode;
    int64_t entered_at_us;
} app_model_t;
void app_model_next(app_model_t *model, int64_t now_us);
// 纯业务计算：不依赖 GPIO、LEDC、FreeRTOS 或 esp_timer。
uint32_t app_model_brightness(const app_model_t *model, int64_t now_us);
