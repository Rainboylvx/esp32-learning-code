#include "key.h"
#include "driver/gpio.h"

// 原理图：BOOT 接 GPIO0，按下接地。原始值、稳定值和变化时间均由扫描任务独占。
static int s_raw = 1, s_stable = 1;
static int64_t s_changed_at;
static bool s_armed;

esp_err_t key_init(void)
{
    gpio_config_t cfg = {.pin_bit_mask = 1ULL << GPIO_NUM_0,
        .mode = GPIO_MODE_INPUT, .pull_up_en = GPIO_PULLUP_ENABLE};
    esp_err_t err = gpio_config(&cfg);
    if (err != ESP_OK) return err;
    s_raw = s_stable = 1;
    s_changed_at = 0;
    // 上电按住 BOOT 时不视为新按下，松开稳定后才允许产生事件。
    s_armed = gpio_get_level(GPIO_NUM_0) == 1;
    return ESP_OK;
}

bool key_poll(int64_t now_us)
{
    int raw = gpio_get_level(GPIO_NUM_0);
    if (raw != s_raw) {
        s_raw = raw;
        s_changed_at = now_us; // 发生任意抖动，都重新计时。
    }
    if (now_us - s_changed_at < 20000) return false;
    if (s_raw == 1) s_armed = true;
    if (s_raw == s_stable) return false;
    s_stable = s_raw;
    if (s_stable == 0 && s_armed) {
        s_armed = false;
        return true;
    }
    return false;
}
