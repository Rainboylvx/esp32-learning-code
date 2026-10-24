#include "app_model.h"
#include "key.h"
#include "pwm_led.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

typedef enum { EVENT_TICK, EVENT_KEY } event_t;
static QueueHandle_t s_events;
static esp_timer_handle_t s_timer;

static void tick_callback(void *arg)
{
    (void)arg;
    event_t event = EVENT_TICK;
    // 默认 esp_timer 任务上下文：普通 API，零等待，队列满时合并刷新机会。
    // 亮度按绝对时间计算，因此不需要补放所有过期的 tick。
    xQueueSend(s_events, &event, 0);
}

static void key_task(void *arg)
{
    (void)arg;
    TickType_t wake = xTaskGetTickCount();
    while (1) {
        if (key_poll(esp_timer_get_time())) {
            event_t event = EVENT_KEY;
            // 普通按键任务允许阻塞；与可合并 tick 不同，模式切换必须保留。
            xQueueSend(s_events, &event, portMAX_DELAY);
        }
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(10));
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(pwm_led_init());
    ESP_ERROR_CHECK(key_init());
    s_events = xQueueCreate(16, sizeof(event_t));
    ESP_ERROR_CHECK(s_events ? ESP_OK : ESP_ERR_NO_MEM);
    ESP_ERROR_CHECK(xTaskCreate(key_task, "key_input", 3072, NULL, 5, NULL)
                    == pdPASS ? ESP_OK : ESP_ERR_NO_MEM);
    esp_timer_create_args_t config = {.callback = tick_callback,
        .dispatch_method = ESP_TIMER_TASK, .name = "refresh"};
    ESP_ERROR_CHECK(esp_timer_create(&config, &s_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(s_timer, 20000));
    app_model_t model = {.mode = APP_OFF, .entered_at_us = esp_timer_get_time()};
    uint32_t last = 0;
    while (1) {
        event_t event;
        xQueueReceive(s_events, &event, portMAX_DELAY);
        int64_t now = esp_timer_get_time();
        // 状态和 PWM 输出只由这个任务修改；生产者只投递事件。
        if (event == EVENT_KEY) {
            app_model_next(&model, now);
            ESP_LOGI("modes", "mode=%d (0=OFF, 1=ON, 2=BREATH)", model.mode);
        }
        uint32_t brightness = app_model_brightness(&model, now);
        if (brightness != last) {
            ESP_ERROR_CHECK(pwm_led_set(brightness));
            last = brightness;
        }
    }
}
