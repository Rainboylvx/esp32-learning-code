#include <stdbool.h>
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// DNESP32S3 V1.2 板载红色用户 LED 连接到 GPIO1。
#define LED_GPIO GPIO_NUM_1

// ESP_LOGI 会把这个字符串作为日志标签打印出来，便于区分日志来源。
static const char *TAG = "led_demo";

/**
 * @brief 设置板载红色 LED 的亮灭状态。
 *
 * 原理图中的连接方式是：3.3 V -> 限流电阻 -> LED -> GPIO1。
 * 因此 GPIO1 输出低电平时形成电流通路，LED 点亮；
 * 输出高电平时两端接近同电位，LED 熄灭。这叫“低电平有效”。
 */
static esp_err_t led_set(bool on)
{
    // 条件表达式等价于：on 为 true 时写 0，否则写 1。
    return gpio_set_level(LED_GPIO, on ? 0 : 1);
}

/**
 * @brief 把 LED 引脚初始化为普通 GPIO 输出，并让 LED 默认熄灭。
 */
static esp_err_t led_init(void)
{
    const gpio_config_t config = {
        // pin_bit_mask 的每一位对应一个 GPIO；左移 GPIO1 位就是选择 GPIO1。
        .pin_bit_mask = 1ULL << LED_GPIO,
        // LED 只需要输出高、低电平，不需要输入功能。
        .mode = GPIO_MODE_OUTPUT,
        // 外部电路已经确定了电平关系，这里不启用芯片内部上下拉电阻。
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        // 点灯不使用 GPIO 中断。
        .intr_type = GPIO_INTR_DISABLE,
    };

    // gpio_config() 返回 ESP_OK 表示配置成功，否则把错误交给调用者处理。
    esp_err_t err = gpio_config(&config);
    if (err != ESP_OK) {
        return err;
    }

    // 初始化结束后先灭灯，避免程序启动时 LED 状态不明确。
    return led_set(false);
}

void app_main(void)
{
    // ESP_ERROR_CHECK 遇到错误会打印错误位置并终止程序，适合入门阶段排错。
    ESP_ERROR_CHECK(led_init());

    // app_main 是一个 FreeRTOS 任务；循环不会退出，LED 会持续闪烁。
    while (1) {
        ESP_ERROR_CHECK(led_set(true));
        ESP_LOGI(TAG, "LED on");
        // 把 500 毫秒换算成当前 FreeRTOS 配置对应的 tick 数。
        vTaskDelay(pdMS_TO_TICKS(500));

        ESP_ERROR_CHECK(led_set(false));
        ESP_LOGI(TAG, "LED off");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
