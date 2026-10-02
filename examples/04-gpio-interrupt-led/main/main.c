#include <stdbool.h>
#include <stdint.h>
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "exit.h"
#include "led.h"

static const char *TAG = "gpio_interrupt_demo";

#define EVENT_QUEUE_LENGTH 8
#define TASK_STACK_SIZE 3072
#define TASK_PRIORITY 5
#define DEBOUNCE_MS 20
#define POLL_MS 10

static void exit_task(void *arg)
{
    (void)arg;
    uint32_t gpio_num;
    bool led_on = false;

    while (1) {
        if (xQueueReceive(exit_event_queue(), &gpio_num, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        if (gpio_num != BOOT_INT_GPIO_PIN) {
            continue;
        }

        // ISR 到达这里只说明发生了边沿，延时和电平确认放在任务上下文。
        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
        if (gpio_get_level(BOOT_INT_GPIO_PIN) != 0) {
            continue;
        }

        led_on = !led_on;
        ESP_ERROR_CHECK(led_set(led_on));
        ESP_LOGI(TAG, "BOOT press accepted, LED %s", led_on ? "on" : "off");

        // 等待按键松开，避免长按或抖动再次产生业务事件。
        while (gpio_get_level(BOOT_INT_GPIO_PIN) == 0) {
            vTaskDelay(pdMS_TO_TICKS(POLL_MS));
        }
        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(led_init());

    QueueHandle_t event_queue = xQueueCreate(EVENT_QUEUE_LENGTH, sizeof(uint32_t));
    ESP_ERROR_CHECK(event_queue != NULL ? ESP_OK : ESP_ERR_NO_MEM);

    ESP_ERROR_CHECK(exit_init(event_queue));
    ESP_ERROR_CHECK(xTaskCreate(exit_task,
                                "exit_task",
                                TASK_STACK_SIZE,
                                NULL,
                                TASK_PRIORITY,
                                NULL) == pdPASS
                        ? ESP_OK
                        : ESP_ERR_NO_MEM);

    ESP_LOGI(TAG, "Ready: press BOOT to toggle LED");
}
