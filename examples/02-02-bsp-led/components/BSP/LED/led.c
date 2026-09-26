#include "led.h"
#include "driver/gpio.h"

// GPIO 编号属于具体开发板的硬件知识，所以放在 BSP 实现中。
#define LED_GPIO GPIO_NUM_1

esp_err_t led_init(void)
{
    const gpio_config_t config = {
        // 位掩码的第 1 位对应 GPIO1。
        .pin_bit_mask = 1ULL << LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        // 板上已有完整的 LED 外部电路，不启用内部上下拉电阻。
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t err = gpio_config(&config);
    if (err != ESP_OK) {
        return err;
    }

    // GPIO 配置成功后先熄灭 LED，让初始化结果确定、可预期。
    return led_set(false);
}

esp_err_t led_set(bool on)
{
    // 原理图为 3.3 V -> 电阻 -> LED -> GPIO1，因此低电平点亮。
    // BSP 在这里完成“逻辑状态”到“硬件电平”的转换。
    return gpio_set_level(LED_GPIO, on ? 0 : 1);
}
