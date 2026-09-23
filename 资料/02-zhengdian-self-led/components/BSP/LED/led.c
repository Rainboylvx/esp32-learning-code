#include "led.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void led_init(void)
{
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << LED_GPIO),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    
    // 初始状态：高电平（LED熄灭，因为是负极驱动）
    gpio_set_level(LED_GPIO, 1);
}

void led_task(void *pvParameters)
{
    int led_state = 0;  // 0=熄灭(高电平), 1=点亮(低电平)
    
    while (1) {
        led_state = !led_state;
        // 负极驱动：低电平点亮，高电平熄灭
        gpio_set_level(LED_GPIO, led_state ? 0 : 1);
        vTaskDelay(pdMS_TO_TICKS(3500));
    }
}
