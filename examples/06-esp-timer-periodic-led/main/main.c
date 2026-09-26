#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"

static const char *TAG = "esp_timer_demo";

// ESP Timer 的周期参数以微秒为单位：1 秒 = 1,000,000 微秒。
#define TIMER_PERIOD_US 1000000ULL

// ESP-IDF 的 xTaskCreate() 栈大小单位是字节。
#define LED_TASK_STACK_SIZE 3072
#define LED_TASK_PRIORITY 5

// 保存句柄，后续若要停止或重启定时器，可以继续操作同一实例。
static esp_timer_handle_t s_periodic_timer;

/**
 * @brief ESP Timer 的周期回调。
 *
 * 默认 ESP_TIMER_TASK 模式下，所有 ESP Timer 回调都在同一个高优先级
 * esp_timer 任务中串行执行。这里不操作 GPIO、不打印日志，也不阻塞，
 * 只发送一个轻量任务通知，把实际工作交给普通任务。
 */
static void periodic_timer_callback(void *arg)
{
    const TaskHandle_t led_task_handle = (TaskHandle_t)arg;
    xTaskNotifyGive(led_task_handle);
}

/**
 * @brief 等待定时通知，翻转 LED，并输出实际处理间隔。
 */
static void led_task(void *arg)
{
    (void)arg;
    bool led_on = false;
    uint32_t total_expirations = 0;
    int64_t previous_time_us = esp_timer_get_time();

    while (1) {
        // 阻塞等待通知，不用轮询占用 CPU。pdTRUE 会在读取后清零计数。
        const uint32_t expirations = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        const int64_t now_us = esp_timer_get_time();
        const int64_t elapsed_us = now_us - previous_time_us;
        previous_time_us = now_us;

        // 如果任务来不及运行，多次通知可能合并成一个大于 1 的计数。
        // 对每次到期都应用一次翻转，才能保留事件次数的奇偶结果。
        for (uint32_t i = 0; i < expirations; ++i) {
            led_on = !led_on;
        }

        total_expirations += expirations;
        ESP_ERROR_CHECK(led_set(led_on));

        ESP_LOGI(TAG,
                 "handled=%" PRIu32 ", total=%" PRIu32
                 ", elapsed=%" PRId64 " us, LED %s",
                 expirations,
                 total_expirations,
                 elapsed_us,
                 led_on ? "on" : "off");
    }
}

void app_main(void)
{
    TaskHandle_t led_task_handle = NULL;

    ESP_ERROR_CHECK(led_init());

    // 必须先创建接收通知的任务，回调参数才能保存一个有效任务句柄。
    ESP_ERROR_CHECK(
        xTaskCreate(led_task,
                    "led_task",
                    LED_TASK_STACK_SIZE,
                    NULL,
                    LED_TASK_PRIORITY,
                    &led_task_handle) == pdPASS
            ? ESP_OK
            : ESP_ERR_NO_MEM);

    const esp_timer_create_args_t timer_args = {
        .callback = periodic_timer_callback,
        .arg = led_task_handle,
        // 默认方式：回调由高优先级 esp_timer 任务统一调度。
        .dispatch_method = ESP_TIMER_TASK,
        .name = "periodic_led",
    };

    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &s_periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(s_periodic_timer, TIMER_PERIOD_US));

    ESP_LOGI(TAG,
             "Started periodic timer: period=%" PRIu64 " us",
             TIMER_PERIOD_US);

    // app_main 返回后会被 ESP-IDF 删除。esp_timer 和 led_task 继续运行。
}
