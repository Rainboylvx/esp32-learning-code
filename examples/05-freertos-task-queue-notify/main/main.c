#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "key.h"
#include "led.h"

static const char *TAG = "freertos_demo";

// 队列最多暂存 8 个尚未处理的按键事件。人的按键速度很慢，
// 这里留出多个位置，是为了让读者看清“队列可以排队”的含义。
#define EVENT_QUEUE_LENGTH 8

// ESP-IDF 中 xTaskCreate() 的栈大小单位是字节。
#define TASK_STACK_SIZE 3072
#define TASK_PRIORITY 5
#define KEY_SCAN_INTERVAL_MS 10

/**
 * @brief 应用层目前只有一种事件，之后可以继续增加其他输入。
 */
typedef enum {
    APP_EVENT_BOOT_PRESSED = 1,
} app_event_type_t;

/**
 * @brief 队列中的一个完整消息。
 *
 * 队列会复制整个结构体，因此生产者可以安全地使用栈上的局部变量。
 */
typedef struct {
    app_event_type_t type;  // 发生了什么。
    TickType_t tick;        // 事件产生时的 FreeRTOS tick，便于观察时序。
} app_event_t;

// 三个任务通过这些句柄找到通信对象或目标任务。
static QueueHandle_t s_event_queue;
static TaskHandle_t s_status_task_handle;

/**
 * @brief 生产者任务：把物理按键动作转换成应用事件。
 */
static void key_task(void *arg)
{
    (void)arg;

    while (1) {
        if (key_scan() == KEY_EVENT_BOOT_PRESS) {
            const app_event_t event = {
                .type = APP_EVENT_BOOT_PRESSED,
                .tick = xTaskGetTickCount(),
            };

            // portMAX_DELAY 表示队列满时阻塞等待，不忙等，也不悄悄丢事件。
            xQueueSend(s_event_queue, &event, portMAX_DELAY);
        }

        // 主动进入阻塞态，让 CPU 可以运行其他任务和系统服务。
        vTaskDelay(pdMS_TO_TICKS(KEY_SCAN_INTERVAL_MS));
    }
}

/**
 * @brief 消费者任务：独占 LED 状态，按队列顺序处理事件。
 */
static void led_task(void *arg)
{
    (void)arg;
    bool led_on = false;
    app_event_t event;

    while (1) {
        // 队列为空时，这个任务进入阻塞态，不会循环占用 CPU。
        if (xQueueReceive(s_event_queue, &event, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        if (event.type == APP_EVENT_BOOT_PRESSED) {
            led_on = !led_on;
            ESP_ERROR_CHECK(led_set(led_on));
            ESP_LOGI(TAG,
                     "queue event: tick=%lu, LED %s",
                     (unsigned long)event.tick,
                     led_on ? "on" : "off");

            // 通知只表达“又处理完一个事件”，不需要再建立一个队列。
            // Give 会把目标任务私有的 32 位通知值加 1。
            xTaskNotifyGive(s_status_task_handle);
        }
    }
}

/**
 * @brief 状态任务：接收轻量通知并累计已处理事件数。
 */
static void status_task(void *arg)
{
    (void)arg;
    uint32_t total_events = 0;

    while (1) {
        // pdTRUE 表示读取后把通知计数清零。若唤醒前累计了多次通知，
        // 返回值会大于 1，因此将返回值累加，而不是固定加 1。
        const uint32_t received = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        total_events += received;

        ESP_LOGI(TAG,
                 "notification: received=%lu, total=%lu",
                 (unsigned long)received,
                 (unsigned long)total_events);
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(led_init());
    ESP_ERROR_CHECK(key_init());

    // 队列保存 8 个 app_event_t 的副本；创建失败通常表示内存不足。
    s_event_queue = xQueueCreate(EVENT_QUEUE_LENGTH, sizeof(app_event_t));
    ESP_ERROR_CHECK(s_event_queue != NULL ? ESP_OK : ESP_ERR_NO_MEM);

    // 先创建 status_task 并取得句柄，led_task 才能向它发送通知。
    ESP_ERROR_CHECK(
        xTaskCreate(status_task,
                    "status_task",
                    TASK_STACK_SIZE,
                    NULL,
                    TASK_PRIORITY,
                    &s_status_task_handle) == pdPASS
            ? ESP_OK
            : ESP_ERR_NO_MEM);

    ESP_ERROR_CHECK(
        xTaskCreate(led_task,
                    "led_task",
                    TASK_STACK_SIZE,
                    NULL,
                    TASK_PRIORITY,
                    NULL) == pdPASS
            ? ESP_OK
            : ESP_ERR_NO_MEM);

    ESP_ERROR_CHECK(
        xTaskCreate(key_task,
                    "key_task",
                    TASK_STACK_SIZE,
                    NULL,
                    TASK_PRIORITY,
                    NULL) == pdPASS
            ? ESP_OK
            : ESP_ERR_NO_MEM);

    ESP_LOGI(TAG, "Ready: press BOOT to send an event");

    // app_main 本身也是一个 FreeRTOS 任务。函数返回后，ESP-IDF 会删除它；
    // 上面创建的三个任务继续由调度器运行。
}
