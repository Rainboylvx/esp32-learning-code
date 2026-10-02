#include <stdbool.h>
#include "key.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// DNESP32S3 V1.2 的 BOOT 按键连接 GPIO0，按下时接地。
#define BOOT_KEY_GPIO GPIO_NUM_0
#define BOOT_KEY_ACTIVE_LEVEL 0
#define BOOT_KEY_RELEASED_LEVEL 1
#define KEY_DEBOUNCE_MS 10

// true 表示已经确认按键处于松开状态，可以接收下一次按下。
static bool s_key_ready = true;

esp_err_t key_init(void)
{
    const gpio_config_t config = {
        .pin_bit_mask = 1ULL << BOOT_KEY_GPIO,
        .mode = GPIO_MODE_INPUT,
        // 原理图未画外部上拉；内部上拉让松开时的 GPIO0 稳定为高电平。
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        // 本篇使用轮询，中断留到后续 EXIT 实验。
        .intr_type = GPIO_INTR_DISABLE,
    };

    s_key_ready = true;
    return gpio_config(&config);
}

key_event_t key_scan(void)
{
    const int level = gpio_get_level(BOOT_KEY_GPIO); // 1 0

    if (s_key_ready && level == BOOT_KEY_ACTIVE_LEVEL) {
        // 首次读到低电平后等待机械触点稳定，再进行第二次确认。
        vTaskDelay(pdMS_TO_TICKS(KEY_DEBOUNCE_MS));

        if (gpio_get_level(BOOT_KEY_GPIO) == BOOT_KEY_ACTIVE_LEVEL) {
            // 锁住本次按下；持续按住时不再重复产生事件。
            s_key_ready = false;
            return KEY_EVENT_BOOT_PRESS;
        }
    } else if (!s_key_ready && level == BOOT_KEY_RELEASED_LEVEL) {
        // 松开同样可能抖动；确认稳定为高电平后再允许下一次按下。
        vTaskDelay(pdMS_TO_TICKS(KEY_DEBOUNCE_MS));

        if (gpio_get_level(BOOT_KEY_GPIO) == BOOT_KEY_RELEASED_LEVEL) {
            s_key_ready = true;
        }
    }

    return KEY_EVENT_NONE;
}
