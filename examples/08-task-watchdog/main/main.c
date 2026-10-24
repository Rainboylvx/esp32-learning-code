#include "sdkconfig.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static void worker(void *arg)
{
    (void)arg;
    // NULL 表示订阅当前任务；喂狗也必须由该任务自己完成。
    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));
    unsigned rounds = 0;
    while (1) {
#if CONFIG_DEMO_WDT_FAULT
        if (rounds == 3) {
            ESP_LOGW("wdt_demo", "FAULT: busy loop on CPU0, no yield / feed");
            int64_t deadline = esp_timer_get_time() + 6000000;
            // 故障注入：中断仍可执行，但同核低优先级 IDLE0 无法运行。
            while (esp_timer_get_time() < deadline) { }
        }
#endif
        // 修复版：把长业务拆成有限批次，并在批次之间阻塞让出 CPU。
        // 真实应用在这里处理一批已就绪的数据，检查业务结果后再喂狗。
        ESP_ERROR_CHECK(esp_task_wdt_reset());
        ESP_LOGI("wdt_demo", "healthy batch %u", ++rounds);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void app_main(void)
{
    // sdkconfig.defaults 禁用自动初始化，让本课独立、明确地配置 TWDT。
    esp_task_wdt_config_t config = {
        .timeout_ms = 3000,
        .idle_core_mask = 1U << 0, // 同时监测 CPU0 空闲任务能否获得运行机会。
        .trigger_panic = true,
    };
    ESP_ERROR_CHECK(esp_task_wdt_init(&config));
    ESP_ERROR_CHECK(xTaskCreatePinnedToCore(worker, "wdt_worker", 3072, NULL,
                     5, NULL, 0) == pdPASS ? ESP_OK : ESP_ERR_NO_MEM);
}
